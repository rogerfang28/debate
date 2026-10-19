#include "RelationBuilder.h"

#include "Log.h"

#include <map>

namespace RelationBuilder {

debate::Relationship fromLinkRow(int linkId,
                                 int fromClaimId,
                                 int toClaimId,
                                 const std::string& connection,
                                 int creatorId,
                                 int linkType,
                                 int debateId,
                                 int relationId,
                                 int toRelationId) {
    debate::Relationship rel;

    // A relation with no from-claims is not a relation -- it is an edge with
    // one end. Refused here rather than downstream, because every consumer
    // reaches for from_claim_ids(0), and proto's repeated-field accessor does
    // NOT bounds-check: reading index 0 of an empty field is undefined
    // behaviour, not an exception. Returning an empty relation makes the
    // caller's `id() == 0` check catch it the same way a missing row does.
    if (fromClaimId <= 0 || toClaimId <= 0) {
        Log::warn("[RelationBuilder] Refusing to build relation " + std::to_string(linkId) +
                  ": from=" + std::to_string(fromClaimId) +
                  " to=" + std::to_string(toClaimId) + " (both must be positive)");
        return rel;
    }

    // --- legacy shape, unchanged for existing readers ---
    debate::Relationship::Link* link = rel.mutable_link();
    link->set_id(linkId);
    link->set_connect_from(fromClaimId);
    link->set_connect_to(toClaimId);
    link->set_connection(connection);
    link->set_creator_id(creatorId);
    link->set_link_type(static_cast<debate::LinkType>(linkType));
    if (debateId != 0) {
        link->set_debate_id(debateId);
    }

    // --- new shape ---
    //
    // THE ENDS ARE NOT ALWAYS THE SAME ENDS. The new shape is semantic: the
    // from-set stands in `role` to the target. The stored row is structural.
    // For one link type those disagree, and it is the single easiest thing to
    // get backwards in this whole migration.
    //
    //   PARENT_CHILD  row runs parent -> child, but the child is evidence
    //                 offered FOR the parent. So the relation runs
    //                 from = [child], to = parent, role = SUPPORTS.
    //                 The ends are SWAPPED relative to the row.
    //
    //   CHALLENGE     row runs challenger -> challenged, which is already the
    //                 direction of attack. from = [challenger],
    //                 to = challenged, role = OPPOSES. NOT swapped.
    //
    // This is also what makes a from-SET meaningful: several claims that
    // together support one conclusion. The reverse -- one conclusion pointing
    // at many pieces of its own evidence -- is not a thing anyone asserts.
    int relFrom = fromClaimId;
    int relTo = toClaimId;
    if (static_cast<debate::LinkType>(linkType) == debate::LinkType::PARENT_CHILD) {
        relFrom = toClaimId;    // the child, which does the supporting
        relTo = fromClaimId;    // the parent, which is supported
    }

    // The relation's identity is its RELATION_ID, not the row's own id. Rows
    // that are their own relation carry RELATION_ID == ID, so this is the same
    // value for them and the distinction only shows up once rows are grouped.
    rel.set_id(relationId > 0 ? relationId : linkId);
    if (debateId != 0) {
        rel.set_debate_id(debateId);
    }
    rel.add_from_claim_ids(relFrom);  // exactly one, until storage widens
    if (toRelationId > 0) {
        // An attack on an argument. The oneof holds the relation and NOT the
        // claim, so a reader asking to_claim_id() gets 0 and to_case() says
        // why -- rather than a claim id that looks like a target and is not.
        rel.set_to_relation_id(toRelationId);
    } else {
        rel.set_to_claim_id(relTo);
    }
    rel.set_creator_id(creatorId);
    rel.set_basis(connection);

    // THREE link types map onto THREE roles, deliberately.
    //
    // The tempting mapping is NORMAL -> SUPPORTS, since NORMAL is vaguely
    // supportive. That would be a silent behaviour change: today every reader
    // is written as
    //
    //     if (PARENT_CHILD) {...} else if (CHALLENGE) {...} else continue;
    //
    // so NORMAL links are DROPPED -- not drawn, not propagated. Folding NORMAL
    // into SUPPORTS destroys the distinction before the reader sees it, and
    // those links would abruptly start rendering and start carrying status.
    // No amount of branching downstream can recover information already lost.
    //
    // ROLE_UNSPECIFIED keeps the three-way split intact, so each reader's
    // else-branch stays exactly as true as it is now.
    switch (static_cast<debate::LinkType>(linkType)) {
        case debate::LinkType::CHALLENGE:
            rel.set_role(debate::OPPOSES);
            break;
        case debate::LinkType::PARENT_CHILD:
            rel.set_role(debate::SUPPORTS);
            break;
        case debate::LinkType::NORMAL:
        default:
            rel.set_role(debate::ROLE_UNSPECIFIED);
            break;
    }

    return rel;
}

std::vector<debate::Relationship> fromLinkRows(const std::vector<LinkRow>& rows,
                                               int debateId,
                                               const std::map<int, int>& relationTargets) {
    std::vector<debate::Relationship> out;
    // Insertion order is preserved so output is stable across calls; a map
    // keyed by relation id would silently reorder the graph between requests.
    std::vector<int> order;
    std::map<int, size_t> indexByRelation;

    for (const LinkRow& row : rows) {
        const int linkId = std::get<0>(row);
        const int from = std::get<1>(row);
        const int to = std::get<2>(row);
        const std::string& connection = std::get<3>(row);
        const int creator = std::get<4>(row);
        const int linkType = std::get<5>(row);
        const int storedRelationId = std::get<6>(row);
        const int relationId = storedRelationId > 0 ? storedRelationId : linkId;
        const auto targetIt = relationTargets.find(linkId);
        const int toRelationId = targetIt == relationTargets.end() ? 0 : targetIt->second;

        auto it = indexByRelation.find(relationId);
        if (it == indexByRelation.end()) {
            debate::Relationship rel = fromLinkRow(linkId, from, to, connection, creator,
                                                   linkType, debateId, relationId, toRelationId);
            if (rel.id() == 0) {
                continue;  // refused: missing an endpoint
            }
            indexByRelation[relationId] = out.size();
            out.push_back(std::move(rel));
            order.push_back(relationId);
            continue;
        }

        // Another from-claim of a relation already seen.
        debate::Relationship& rel = out[it->second];

        // The row's own ends may need the same swap fromLinkRow applies, so go
        // through it rather than reaching for `from` directly -- that swap is
        // exactly the thing that must not be reimplemented in two places.
        const debate::Relationship extra = fromLinkRow(linkId, from, to, connection, creator,
                                                       linkType, debateId, relationId, toRelationId);
        if (extra.id() == 0 || extra.from_claim_ids_size() == 0) {
            continue;
        }

        if (extra.to_claim_id() != rel.to_claim_id() ||
            extra.to_relation_id() != rel.to_relation_id() || extra.role() != rel.role()) {
            Log::warn("[RelationBuilder] Rows in relation " + std::to_string(relationId) +
                      " disagree about target or role; keeping the first. Link " +
                      std::to_string(linkId) + " ignored.");
            continue;
        }

        const int addition = extra.from_claim_ids(0);
        bool duplicate = false;
        for (const int existing : rel.from_claim_ids()) {
            if (existing == addition) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate) {
            rel.add_from_claim_ids(addition);
        }
    }

    return out;
}

}  // namespace RelationBuilder
