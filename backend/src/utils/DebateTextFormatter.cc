#include "DebateTextFormatter.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <vector>

namespace {

std::string nameFor(int userId, const std::map<int, std::string>& usernames) {
    auto it = usernames.find(userId);
    if (it != usernames.end() && !it->second.empty()) {
        return it->second;
    }
    return "User " + std::to_string(userId);
}

std::string indent(int depth) {
    return std::string(static_cast<size_t>(depth) * 2, ' ');
}

// How the viewer stands on a claim, plus any disagreement.
//
// Status is per-user by design -- two people can hold opposite views of the
// same claim and both be correctly recorded. Reporting only the viewer's view
// would hide exactly the disagreement a summary should be describing, so
// dissent is spelled out instead of averaged away.
std::string statusPhrase(const debate::Claim& claim, const std::string& viewerUsername) {
    auto describe = [](debate::ClaimStatus status) -> std::string {
        switch (status) {
            case debate::TRUE_CLAIM:  return "accepted";
            case debate::FALSE_CLAIM: return "refuted";
            default:                  return "open";
        }
    };

    debate::ClaimStatus viewerStatus = debate::UNDETERMINED;
    auto viewerIt = claim.user_statuses().find(viewerUsername);
    if (viewerIt != claim.user_statuses().end()) {
        viewerStatus = viewerIt->second;
    }

    std::vector<std::string> dissent;
    for (const auto& entry : claim.user_statuses()) {
        if (entry.first == viewerUsername) {
            continue;
        }
        if (entry.second != viewerStatus) {
            dissent.push_back(entry.first + " has it " + describe(entry.second));
        }
    }

    std::string phrase = describe(viewerStatus);
    if (!dissent.empty()) {
        phrase += "; DISPUTED -- ";
        for (size_t i = 0; i < dissent.size(); ++i) {
            if (i > 0) {
                phrase += ", ";
            }
            phrase += dissent[i];
        }
    }
    return phrase;
}

// "(evidential)" and friends. Placeholders written by the pre-basis code are
// not bases and must not be presented as though somebody chose them.
std::string basisSuffix(const debate::Relationship& relation) {
    const std::string& basis = relation.basis();
    if (basis.empty() || basis == "parent to child connection" || basis == "challenge link") {
        return "";
    }
    std::string lowered = basis;
    std::transform(lowered.begin(), lowered.end(), lowered.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return " (" + lowered + ")";
}

struct Renderer {
    const debate::Collection& collection;
    const std::map<int, std::string>& usernames;
    std::string viewer;
    std::ostringstream out;
    std::set<int> renderedClaims;

    // target claim id -> relations pointing AT it.
    std::map<int, std::vector<const debate::Relationship*>> incoming;
    // target relation id -> relations attacking that relation's reasoning.
    std::map<int, std::vector<const debate::Relationship*>> againstRelation;

    Renderer(const debate::Collection& c,
             const std::map<int, std::string>& u,
             const std::string& v)
        : collection(c), usernames(u), viewer(v) {
        for (const auto& entry : collection.links_by_id()) {
            const debate::Relationship& rel = entry.second;
            if (rel.to_case() == debate::Relationship::kToClaimId) {
                incoming[rel.to_claim_id()].push_back(&rel);
            } else if (rel.to_case() == debate::Relationship::kToRelationId) {
                againstRelation[rel.to_relation_id()].push_back(&rel);
            }
        }
    }

    void writeClaim(int claimId, int depth) {
        auto claimIt = collection.claims_by_id().find(claimId);
        if (claimIt == collection.claims_by_id().end()) {
            out << indent(depth) << "- [#" << claimId << "] (claim not available)\n";
            return;
        }
        const debate::Claim& claim = claimIt->second;

        // A claim can legitimately be reached twice, because it may support two
        // different parents. Re-rendering its whole subtree would inflate the
        // debate, and would not terminate at all if the graph holds a cycle.
        const bool repeat = !renderedClaims.insert(claimId).second;

        out << indent(depth) << "- [#" << claimId << "] " << claim.sentence() << "\n";
        out << indent(depth) << "  by " << nameFor(claim.creator_id(), usernames)
            << "; " << statusPhrase(claim, viewer) << "\n";
        if (!claim.description().empty()) {
            out << indent(depth) << "  note: " << claim.description() << "\n";
        }
        if (repeat) {
            out << indent(depth) << "  (already shown above; not repeated)\n";
            return;
        }

        writeChildren(claimId, depth + 1);
    }

    void writeChildren(int claimId, int depth) {
        auto it = incoming.find(claimId);
        if (it == incoming.end()) {
            return;
        }
        // Supports before challenges, so the case is stated before the attack.
        for (int pass = 0; pass < 2; ++pass) {
            const debate::RelationRole wanted = pass == 0 ? debate::SUPPORTS : debate::OPPOSES;
            for (const debate::Relationship* rel : it->second) {
                if (rel->role() == wanted) {
                    writeRelation(*rel, depth);
                }
            }
        }
    }

    void writeRelation(const debate::Relationship& rel, int depth) {
        const bool supports = rel.role() == debate::SUPPORTS;
        const bool joint = rel.from_claim_ids_size() > 1;

        out << indent(depth) << (supports ? "SUPPORTED BY" : "CHALLENGED BY") << basisSuffix(rel);
        if (joint) {
            // The distinction a flat list would destroy: these claims are ONE
            // inference that fails if any of them fails, not several
            // independent reasons that survive each other.
            out << " -- all of the following TOGETHER";
        }
        out << ":\n";

        for (int fromId : rel.from_claim_ids()) {
            writeClaim(fromId, depth + 1);
        }

        auto attacks = againstRelation.find(rel.id());
        if (attacks != againstRelation.end()) {
            for (const debate::Relationship* attack : attacks->second) {
                out << indent(depth + 1) << "CHALLENGE AGAINST THE REASONING ABOVE"
                    << basisSuffix(*attack) << " (not against the claims themselves):\n";
                for (int fromId : attack->from_claim_ids()) {
                    writeClaim(fromId, depth + 2);
                }
            }
        }
    }
};

} // namespace

std::string DebateTextFormatter::Format(const debate::Collection& collection,
                                        const std::map<int, std::string>& usernames,
                                        const Options& options) {
    Renderer renderer(collection, usernames, options.viewerUsername);

    // Roots are the claims never offered in support of anything else. They are
    // what the debate is ultimately about.
    std::set<int> isSource;
    for (const auto& entry : collection.links_by_id()) {
        for (int fromId : entry.second.from_claim_ids()) {
            isSource.insert(fromId);
        }
    }

    std::vector<int> roots;
    for (const auto& entry : collection.claims_by_id()) {
        if (isSource.count(entry.first) == 0) {
            roots.push_back(entry.first);
        }
    }
    std::sort(roots.begin(), roots.end());

    renderer.out << "DEBATE";
    if (!options.topic.empty()) {
        renderer.out << ": " << options.topic;
    }
    renderer.out << "\n";
    renderer.out << "Each claim shows its id, its author, and whether "
                 << (options.viewerUsername.empty() ? "the viewer" : options.viewerUsername)
                 << " currently accepts it.\n";
    renderer.out << "Indentation means the indented block is offered FOR or AGAINST "
                    "the line above it.\n\n";

    if (collection.claims_by_id().empty()) {
        renderer.out << "(this debate has no claims yet)\n";
        return renderer.out.str();
    }

    for (int rootId : roots) {
        renderer.writeClaim(rootId, 0);
        renderer.out << "\n";
    }

    // Anything the walk never reached: a claim whose relation row is missing,
    // or a cycle with no root. Dropping these silently would hand the model an
    // incomplete debate that looks complete, so they are stated as detached
    // rather than omitted.
    std::vector<int> orphans;
    for (const auto& entry : collection.claims_by_id()) {
        if (renderer.renderedClaims.count(entry.first) == 0) {
            orphans.push_back(entry.first);
        }
    }
    if (!orphans.empty()) {
        std::sort(orphans.begin(), orphans.end());
        renderer.out << "NOT CONNECTED TO ANY ROOT (shown so nothing is omitted):\n";
        for (int orphanId : orphans) {
            renderer.writeClaim(orphanId, 1);
        }
    }

    return renderer.out.str();
}

std::string DebateTextFormatter::FormatClaimContext(const debate::Collection& collection,
                                                    const std::map<int, std::string>& usernames,
                                                    const Options& options,
                                                    int claimId) {
    Renderer renderer(collection, usernames, options.viewerUsername);

    auto focusIt = collection.claims_by_id().find(claimId);
    if (focusIt == collection.claims_by_id().end()) {
        return "The selected claim is not part of this debate.";
    }
    const debate::Claim& focus = focusIt->second;

    // Where the claim sits: follow it upward until nothing carries it further.
    // This is what turns "some sentence" into "a step in an argument for X".
    std::vector<const debate::Relationship*> upward;
    std::set<int> seenUp;
    int current = claimId;
    while (seenUp.insert(current).second) {
        const debate::Relationship* next = nullptr;
        for (const auto& entry : collection.links_by_id()) {
            const debate::Relationship& rel = entry.second;
            if (rel.to_case() != debate::Relationship::kToClaimId) {
                continue;
            }
            for (int fromId : rel.from_claim_ids()) {
                if (fromId == current) {
                    next = &rel;
                    break;
                }
            }
            if (next != nullptr) {
                break;
            }
        }
        if (next == nullptr) {
            break;
        }
        upward.push_back(next);
        current = next->to_claim_id();
    }

    renderer.out << "DEBATE";
    if (!options.topic.empty()) {
        renderer.out << ": " << options.topic;
    }
    renderer.out << "\n\n";

    renderer.out << "THE CLAIM IN FOCUS\n";
    renderer.out << "- [#" << focus.id() << "] " << focus.sentence() << "\n";
    renderer.out << "  by " << nameFor(focus.creator_id(), usernames) << "; "
                 << statusPhrase(focus, options.viewerUsername) << "\n";
    if (!focus.description().empty()) {
        renderer.out << "  note: " << focus.description() << "\n";
    }
    renderer.out << "\n";

    renderer.out << "WHAT THIS CLAIM IS DOING IN THE DEBATE\n";
    if (upward.empty()) {
        renderer.out << "Nothing is built on this claim -- it is a top-level claim "
                        "of the debate, not a step toward another one.\n";
    } else {
        int depth = 0;
        for (const debate::Relationship* rel : upward) {
            auto parentIt = collection.claims_by_id().find(rel->to_claim_id());
            const std::string verb =
                rel->role() == debate::OPPOSES ? "is offered AGAINST" : "is offered IN SUPPORT OF";
            renderer.out << indent(depth) << verb << basisSuffix(*rel) << ":\n";
            if (parentIt == collection.claims_by_id().end()) {
                renderer.out << indent(depth + 1) << "- [#" << rel->to_claim_id()
                             << "] (claim not available)\n";
            } else {
                renderer.out << indent(depth + 1) << "- [#" << parentIt->second.id() << "] "
                             << parentIt->second.sentence() << "\n";
                renderer.out << indent(depth + 1) << "  by "
                             << nameFor(parentIt->second.creator_id(), usernames) << "; "
                             << statusPhrase(parentIt->second, options.viewerUsername) << "\n";
            }
            // Siblings matter: if this claim is one part of a joint argument,
            // then on its own it does not carry the parent at all, and a
            // summary that omitted that would overstate what it does.
            if (rel->from_claim_ids_size() > 1) {
                renderer.out << indent(depth + 1)
                             << "NOTE: this step only works TOGETHER with:\n";
                for (int fromId : rel->from_claim_ids()) {
                    if (fromId == claimId) {
                        continue;
                    }
                    auto siblingIt = collection.claims_by_id().find(fromId);
                    if (siblingIt != collection.claims_by_id().end()) {
                        renderer.out << indent(depth + 2) << "- [#" << fromId << "] "
                                     << siblingIt->second.sentence() << "\n";
                    }
                }
                renderer.out << indent(depth + 1)
                             << "If any one of them fails, this whole step fails.\n";
            }
            depth += 1;
        }
    }
    renderer.out << "\n";

    renderer.out << "WHAT IS OFFERED FOR AND AGAINST THIS CLAIM\n";
    renderer.renderedClaims.insert(claimId);
    const size_t before = renderer.out.str().size();
    renderer.writeChildren(claimId, 0);
    if (renderer.out.str().size() == before) {
        renderer.out << "Nothing has been offered for or against it yet.\n";
    }

    return renderer.out.str();
}
