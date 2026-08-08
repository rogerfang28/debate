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

// ---------------------------------------
// PARITY MODE
// ---------------------------------------

namespace {

// Pulls an integer out of the crude JSON the move logger writes, e.g.
// {"challenging_claim_id":42}. A real parser is not warranted: MoveLogger is
// the only writer, the shapes are fixed and flat, and every caller below
// tolerates a miss by skipping the seed rather than guessing one.
int payloadInt(const std::string& payload, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    const size_t at = payload.find(needle);
    if (at == std::string::npos) {
        return 0;
    }
    size_t i = at + needle.size();
    while (i < payload.size() && (payload[i] == ' ' || payload[i] == '"')) {
        ++i;
    }
    int value = 0;
    bool any = false;
    while (i < payload.size() && payload[i] >= '0' && payload[i] <= '9') {
        value = value * 10 + (payload[i] - '0');
        ++i;
        any = true;
    }
    return any ? value : 0;
}

// The legacy graph indexes, built from the same link shape the old engine reads.
struct LegacyGraph {
    std::map<int, std::vector<int>> children;           // parent -> child ids
    std::map<int, std::vector<int>> challengeIncoming;  // target -> challenger ids
    std::map<int, std::vector<int>> challengeOutgoing;  // challenger -> target ids
};

LegacyGraph buildLegacyGraph(const std::vector<debate::Relationship>& relations) {
    LegacyGraph g;
    for (const auto& rel : relations) {
        if (rel.from_claim_ids_size() == 0) {
            continue;  // RelationBuilder refuses these; a producer that didn't
        }

        // Relation-targeted relations are R3 and have no place in this graph:
        // it is a claim->claim index by construction. Skipped explicitly rather
        // than by falling through, because to_claim_id() on a relation-targeted
        // relation quietly returns 0 -- it would look like an edge to claim 0
        // rather than like something unhandled.
        if (rel.to_case() != debate::Relationship::kToClaimId) {
            continue;
        }
        const int to = rel.to_claim_id();
        if (to <= 0) {
            continue;
        }

        // Every member of the from-set relates to the target independently, so
        // a multi-from relation contributes one edge per member. Today that is
        // always one edge; writing the loop now means widening storage does not
        // require revisiting this.
        for (const int from : rel.from_claim_ids()) {
            if (from <= 0) {
                continue;
            }
            // ROLE_UNSPECIFIED (legacy NORMAL) falls through both branches and
            // is dropped, exactly as the link_type version dropped it.
            if (rel.role() == debate::SUPPORTS) {
                // `children` is keyed PARENT -> children, and a SUPPORTS
                // relation runs child -> parent, so the ends go in reversed.
                // This map feeds Rule A/B, which ask "what are this claim's
                // children?" -- getting it backwards would make every claim
                // the child of its own evidence.
                g.children[to].push_back(from);
            } else if (rel.role() == debate::OPPOSES) {
                // Attacks already run challenger -> challenged; no swap.
                g.challengeIncoming[to].push_back(from);
                g.challengeOutgoing[from].push_back(to);
            }
        }
    }
    return g;
}

// A working copy of user_statuses: claim id -> username -> status.
using StatusTable = std::map<int, std::map<std::string, debate::ClaimStatus>>;

debate::ClaimStatus lookup(const StatusTable& table, int claimId, const std::string& user,
                           bool* found) {
    auto cit = table.find(claimId);
    if (cit != table.end()) {
        auto uit = cit->second.find(user);
        if (uit != cit->second.end()) {
            *found = true;
            return uit->second;
        }
    }
    *found = false;
    return debate::ClaimStatus::UNDETERMINED;
}

// A line-by-line copy of DebateWrapper::ComputeStatusForUser. Kept structurally
// identical -- same rule order, same early breaks, same "missing entry means not
// all-true" handling -- because the point is to detect where it disagrees, and a
// tidier reimplementation would only produce differences that prove nothing.
debate::ClaimStatus computeForUser(int claimId, const std::string& user,
                                   debate::ClaimStatus currentStatus, const LegacyGraph& g,
                                   const std::set<int>& conceded, const StatusTable& table) {
    debate::ClaimStatus ns = currentStatus;

    // Rule A/B: parent-child.
    bool anyChildFalse = false;
    bool allChildrenTrue = true;
    auto childrenIt = g.children.find(claimId);
    const bool hasChildren = childrenIt != g.children.end() && !childrenIt->second.empty();
    if (childrenIt != g.children.end()) {
        for (int childId : childrenIt->second) {
            bool found = false;
            const debate::ClaimStatus cs = lookup(table, childId, user, &found);
            if (!found) {
                allChildrenTrue = false;
                continue;
            }
            if (cs == debate::ClaimStatus::FALSE_CLAIM) {
                anyChildFalse = true;
                break;
            }
            if (cs != debate::ClaimStatus::TRUE_CLAIM) {
                allChildrenTrue = false;
            }
        }
    }
    if (anyChildFalse) {
        ns = debate::ClaimStatus::FALSE_CLAIM;
    } else if (hasChildren && allChildrenTrue && conceded.find(claimId) == conceded.end()) {
        ns = debate::ClaimStatus::TRUE_CLAIM;
    }

    auto citIn = g.challengeIncoming.find(claimId);

    // Rule C: an incoming challenge the user believes FALSE vindicates this claim.
    if (citIn != g.challengeIncoming.end()) {
        for (int challengerId : citIn->second) {
            bool found = false;
            if (lookup(table, challengerId, user, &found) == debate::ClaimStatus::FALSE_CLAIM &&
                found) {
                ns = debate::ClaimStatus::TRUE_CLAIM;
                break;
            }
        }
    }

    // Rule D: an incoming challenge the user believes TRUE defeats this claim.
    // Runs after C, so when both fire D wins -- that ordering is the old
    // engine's tie-break and is load-bearing.
    if (citIn != g.challengeIncoming.end()) {
        for (int challengerId : citIn->second) {
            bool found = false;
            if (lookup(table, challengerId, user, &found) == debate::ClaimStatus::TRUE_CLAIM &&
                found) {
                ns = debate::ClaimStatus::FALSE_CLAIM;
                break;
            }
        }
    }

    return ns;
}

}  // namespace

LegacyResult computeLegacyStatuses(const std::vector<debate::Claim>& claims,
                                   const std::vector<debate::Relationship>& relations,
                                   const std::vector<debate::Move>& moves,
                                   const std::map<int, std::string>& usernames) {
    LegacyResult result;

    std::set<int> known;
    for (const auto& c : claims) {
        if (c.id() > 0) {
            known.insert(c.id());
        }
    }

    const LegacyGraph g = buildLegacyGraph(relations);

    std::vector<const debate::Move*> ordered;
    ordered.reserve(moves.size());
    for (const auto& m : moves) {
        ordered.push_back(&m);
    }
    std::stable_sort(
        ordered.begin(), ordered.end(),
        [](const debate::Move* a, const debate::Move* b) { return a->seq() < b->seq(); });

    // ---- Seed ----
    //
    // Replay only the DIRECT writes the handlers make to user_statuses. The
    // propagation pass below reproduces everything else, so seeding anything
    // derived here would double-count it.
    StatusTable table;
    std::set<int> conceded;

    auto put = [&](int claimId, const std::string& user, debate::ClaimStatus s) {
        if (claimId > 0 && known.count(claimId) && !user.empty()) {
            table[claimId][user] = s;
        }
    };

    for (const debate::Move* m : ordered) {
        auto nameIt = usernames.find(m->actor_id());
        if (nameIt == usernames.end() || nameIt->second.empty()) {
            continue;  // cannot attribute a per-user view without a username
        }
        const std::string& user = nameIt->second;
        const int target = m->target_id();

        switch (m->type()) {
            case debate::ASSERT:
                // addClaimToDB and friends: the creator sees their own claim TRUE.
                // Relation asserts write no status.
                if (m->target_type() == debate::TARGET_CLAIM) {
                    put(target, user, debate::ClaimStatus::TRUE_CLAIM);
                }
                break;

            case debate::OPPOSE:
                // SubmitChallengeClaim: the challenger sees the target FALSE.
                // Their own challenge claim was already seeded by its ASSERT.
                if (m->target_type() == debate::TARGET_CLAIM) {
                    put(target, user, debate::ClaimStatus::FALSE_CLAIM);
                }
                break;

            case debate::CONCEDE: {
                // ConcedeChallenge writes four groups for the conceder.
                const int challengingClaimId = payloadInt(m->payload(), "challenging_claim_id");

                put(challengingClaimId, user, debate::ClaimStatus::TRUE_CLAIM);  // step 1
                put(target, user, debate::ClaimStatus::FALSE_CLAIM);             // step 1b

                // Step 1c: every PARENT_CHILD descendant of the challenge claim.
                // Conceding accepts the whole supporting chain, not just its head.
                if (challengingClaimId > 0) {
                    std::set<int> visited{challengingClaimId};
                    std::vector<int> queue{challengingClaimId};
                    while (!queue.empty()) {
                        const int current = queue.back();
                        queue.pop_back();
                        auto it = g.children.find(current);
                        if (it == g.children.end()) {
                            continue;
                        }
                        for (int childId : it->second) {
                            if (visited.insert(childId).second) {
                                put(childId, user, debate::ClaimStatus::TRUE_CLAIM);
                                queue.push_back(childId);
                            }
                        }
                    }
                }

                // Step 2: anything challenging the challenge claim is marked TRUE.
                auto inIt = g.challengeIncoming.find(challengingClaimId);
                if (inIt != g.challengeIncoming.end()) {
                    for (int challengerId : inIt->second) {
                        put(challengerId, user, debate::ClaimStatus::TRUE_CLAIM);
                    }
                }

                // Step 3 passes the CHALLENGE claim as conceded, not the target,
                // which is what blocks Rule B from re-deriving it.
                if (challengingClaimId > 0) {
                    conceded.insert(challengingClaimId);
                }
                break;
            }

            case debate::RETRACT:
            default:
                // RETRACT writes no status: the handler removes the row and its
                // links, so the effect already shows up in `claims`/`relations`.
                break;
        }
    }

    // ---- Propagate ----
    //
    // BFS order from the root, then any claim the BFS never reached. The old
    // engine takes the root from the Debate row; here it is inferred as the
    // claim that is nobody's child, which agrees for every debate with a single
    // root and is only an ordering hint in any case -- the loop runs to a fixed
    // point, so order affects how many passes it takes, not where it lands.
    std::set<int> isChild;
    for (const auto& entry : g.children) {
        for (int childId : entry.second) {
            isChild.insert(childId);
        }
    }
    std::vector<int> order;
    std::set<int> placed;
    for (int id : known) {
        if (!isChild.count(id)) {
            order.push_back(id);
            placed.insert(id);
        }
    }
    for (size_t i = 0; i < order.size(); ++i) {
        auto it = g.children.find(order[i]);
        if (it == g.children.end()) {
            continue;
        }
        for (int childId : it->second) {
            if (known.count(childId) && placed.insert(childId).second) {
                order.push_back(childId);
            }
        }
    }
    for (int id : known) {
        if (placed.insert(id).second) {
            order.push_back(id);
        }
    }

    // Fixed point, capped at 50 passes exactly as the old engine caps it.
    // Writes land in `table` immediately, so a later claim in the same pass sees
    // an earlier one's new value. That in-place behaviour is not incidental --
    // the old engine re-reads each claim from the database mid-pass, so copying
    // it into a snapshot here would converge differently.
    bool changed = true;
    int maxIterations = 50;
    while (changed && maxIterations-- > 0) {
        changed = false;

        for (int claimId : order) {
            // The old engine recomputes the user set per pass: a user is
            // considered for this claim if they hold an opinion on it, on any
            // child, or on either end of a challenge touching it. This is how a
            // viewpoint spreads outward from the claims someone actually acted
            // on, and narrowing it to seeded users alone would under-propagate.
            std::set<std::string> users;
            auto collect = [&](int otherId) {
                auto it = table.find(otherId);
                if (it != table.end()) {
                    for (const auto& e : it->second) {
                        users.insert(e.first);
                    }
                }
            };
            collect(claimId);
            auto kids = g.children.find(claimId);
            if (kids != g.children.end()) {
                for (int id : kids->second) collect(id);
            }
            auto in = g.challengeIncoming.find(claimId);
            if (in != g.challengeIncoming.end()) {
                for (int id : in->second) collect(id);
            }
            auto out = g.challengeOutgoing.find(claimId);
            if (out != g.challengeOutgoing.end()) {
                for (int id : out->second) collect(id);
            }

            for (const std::string& user : users) {
                bool found = false;
                const debate::ClaimStatus cs = lookup(table, claimId, user, &found);
                const debate::ClaimStatus ns =
                    computeForUser(claimId, user, cs, g, conceded, table);
                if (ns != cs) {
                    table[claimId][user] = ns;
                    changed = true;
                }
            }
        }
    }

    // Transpose into username -> claim id -> status, the shape the stored
    // per-user matrix is compared against.
    for (const auto& claimEntry : table) {
        for (const auto& userEntry : claimEntry.second) {
            result.by_user[userEntry.first][claimEntry.first] = userEntry.second;
        }
    }

    return result;
}

}  // namespace StatusEngine
