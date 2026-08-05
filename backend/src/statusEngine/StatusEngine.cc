#include "StatusEngine.h"

#include <algorithm>
#include <deque>
#include <set>

namespace {

// One directed edge: `supporter` props up `supported`. Collapse travels along
// these, from supporter to supported.
struct SupportEdge {
    int supporter = 0;
    int supported = 0;
};

// Reads support edges out of whichever shape a relation is stored in.
//
// Two shapes coexist during the migration:
//
//   LEGACY -- Relationship.link, a single connect_from -> connect_to pair.
//     A PARENT_CHILD link runs parent -> child, but the MEANING is the
//     reverse: the child is evidence offered for the parent. So the support
//     edge is child -> parent, and knocking out a child damages the parent.
//     This inversion is the single easiest thing to get backwards here.
//
//     CHALLENGE links are opposition, not support, and are skipped: a
//     collapsing challenge should let its target recover, never drag it down.
//     Modelling that needs OPEN/UNSUPPORTED, which are out of scope.
//
//     NORMAL links carry no agreed meaning in the current model -- nothing
//     computes status from them -- so they are skipped rather than guessed at.
//
//   NEW -- from_claim_ids -> to_claim_id with an explicit role. Every member
//     of the from-set supports the target. Only SUPPORTS contributes.
//     A relation targeting another relation is ignored here: that is R3, and
//     it produces UNSUPPORTED, which this subset cannot express.
std::vector<SupportEdge> supportEdgesOf(const debate::Relationship& rel) {
    std::vector<SupportEdge> edges;

    // New shape takes precedence when populated.
    if (rel.from_claim_ids_size() > 0) {
        if (rel.role() != debate::SUPPORTS) {
            return edges;
        }
        if (rel.to_case() != debate::Relationship::kToClaimId) {
            return edges;  // relation-targeted: R3, not this subset
        }
        for (int from : rel.from_claim_ids()) {
            if (from > 0 && rel.to_claim_id() > 0) {
                edges.push_back({from, rel.to_claim_id()});
            }
        }
        return edges;
    }

    // Legacy shape.
    const debate::Relationship::Link& link = rel.link();
    if (link.id() == 0) {
        return edges;
    }
    if (link.link_type() != debate::LinkType::PARENT_CHILD) {
        return edges;
    }
    const int parent = link.connect_from();
    const int child = link.connect_to();
    if (parent > 0 && child > 0) {
        edges.push_back({child, parent});  // child supports parent
    }
    return edges;
}

// One directed edge: `attacker` opposes `attacked`.
//
// Used only to decide whether an attack is still live. A challenge whose own
// claim has collapsed -- conceded or withdrawn -- stops holding its target
// open, which is how a target recovers when its challenger walks away.
struct OppositionEdge {
    int attacker = 0;
    int attacked = 0;
};

std::vector<OppositionEdge> oppositionEdgesOf(const debate::Relationship& rel) {
    std::vector<OppositionEdge> edges;

    if (rel.from_claim_ids_size() > 0) {
        if (rel.role() != debate::OPPOSES) {
            return edges;
        }
        if (rel.to_case() != debate::Relationship::kToClaimId) {
            return edges;  // relation-targeted attack: R3, produces UNSUPPORTED
        }
        for (int from : rel.from_claim_ids()) {
            if (from > 0 && rel.to_claim_id() > 0) {
                edges.push_back({from, rel.to_claim_id()});
            }
        }
        return edges;
    }

    const debate::Relationship::Link& link = rel.link();
    if (link.id() == 0 || link.link_type() != debate::LinkType::CHALLENGE) {
        return edges;
    }
    const int challenger = link.connect_from();
    const int challenged = link.connect_to();
    if (challenger > 0 && challenged > 0) {
        edges.push_back({challenger, challenged});
    }
    return edges;
}

}  // namespace

namespace StatusEngine {

Result computeStatuses(const std::vector<debate::Claim>& claims,
                       const std::vector<debate::Relationship>& relations,
                       const std::vector<debate::Move>& moves) {
    Result result;

    // Every claim starts STANDING. That is the burden rule: an unattacked
    // claim stands, bare assertion included. It does NOT mean verified -- it
    // means nobody has knocked it down yet, and the renderer must say so.
    std::set<int> known;
    for (const auto& c : claims) {
        if (c.id() > 0) {
            result.claim_status[c.id()] = debate::STANDING;
            known.insert(c.id());
        }
    }

    // Replay in sequence order. Callers are not required to sort, and a log
    // applied out of order is a different log.
    std::vector<const debate::Move*> ordered;
    ordered.reserve(moves.size());
    for (const auto& m : moves) {
        ordered.push_back(&m);
    }
    std::stable_sort(ordered.begin(), ordered.end(),
                     [](const debate::Move* a, const debate::Move* b) {
                         return a->seq() < b->seq();
                     });

    // Fold the log into the set of claims defeated outright.
    //
    // CONCEDE -- the author gives the claim up: "you defeated this".
    // RETRACT -- the author withdraws it: "I am taking this back".
    // Different meanings to a reader, and they will render differently, but
    // both remove the claim from play, so both land on COLLAPSED here.
    //
    // ASSERT is read and needs no action: it is what put the claim in `claims`
    // in the first place, and STANDING is already the default.
    //
    // OPPOSE marks its target as under challenge. It never defeats anything on
    // its own: an attack has no mechanical success condition, and only bites
    // when someone concedes or times out. Treating an unanswered OPPOSE as a
    // defeat would be the tool declaring a winner, which it must never do.
    std::deque<int> collapsed;
    std::set<int> seen;
    std::set<int> challenged;

    for (const debate::Move* m : ordered) {
        if (m->target_type() != debate::TARGET_CLAIM) {
            continue;  // relation targets need UNSUPPORTED; out of scope
        }
        const int target = m->target_id();
        if (target <= 0 || !known.count(target)) {
            continue;  // a move about a claim we were not given
        }

        switch (m->type()) {
            case debate::CONCEDE:
            case debate::RETRACT:
                if (seen.insert(target).second) {
                    collapsed.push_back(target);
                }
                break;

            case debate::OPPOSE:
                challenged.insert(target);
                break;

            case debate::ASSERT:
            default:
                break;
        }
    }

    // Build the supporter -> supported index once.
    std::map<int, std::vector<int>> supports;
    for (const auto& rel : relations) {
        for (const auto& e : supportEdgesOf(rel)) {
            if (known.count(e.supporter) && known.count(e.supported)) {
                supports[e.supporter].push_back(e.supported);
            }
        }
    }

    // Cascade. Breadth-first from the directly-defeated claims, following
    // support edges upward. `seen` doubles as the visited set, so a cycle in
    // the relation graph terminates instead of spinning -- users can create
    // loops, so this cannot be assumed away.
    while (!collapsed.empty()) {
        const int id = collapsed.front();
        collapsed.pop_front();
        result.claim_status[id] = debate::COLLAPSED;

        auto it = supports.find(id);
        if (it == supports.end()) {
            continue;
        }
        for (int dependent : it->second) {
            if (seen.insert(dependent).second) {
                collapsed.push_back(dependent);
            }
        }
    }

    // OPEN: under challenge, unresolved.
    //
    // Applied after the cascade because COLLAPSED outranks OPEN on the
    // precedence ladder -- a claim that has been given up is settled, not still
    // in dispute, no matter how many challenges also point at it.
    //
    // An attack only counts while its own claim still stands. A challenge that
    // was conceded or withdrawn stops holding its target open, which is how a
    // target recovers when its challenger walks away. Where no attacking claim
    // can be identified the attack is treated as live: the log says a challenge
    // was raised, so the safe reading is that it still stands.
    std::map<int, std::vector<int>> attackers;
    for (const auto& rel : relations) {
        for (const auto& e : oppositionEdgesOf(rel)) {
            if (known.count(e.attacked)) {
                attackers[e.attacked].push_back(e.attacker);
            }
        }
    }

    for (int target : challenged) {
        if (result.claim_status[target] == debate::COLLAPSED) {
            continue;
        }
        auto it = attackers.find(target);
        if (it == attackers.end()) {
            result.claim_status[target] = debate::OPEN;
            continue;
        }
        for (int attacker : it->second) {
            const bool attackerAlive =
                !known.count(attacker) ||
                result.claim_status[attacker] != debate::COLLAPSED;
            if (attackerAlive) {
                result.claim_status[target] = debate::OPEN;
                break;
            }
        }
    }

    return result;
}

}  // namespace StatusEngine
