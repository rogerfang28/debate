#include "RelationBuilder.h"

#include "Log.h"

namespace RelationBuilder {

debate::Relationship fromLinkRow(int linkId,
                                 int fromClaimId,
                                 int toClaimId,
                                 const std::string& connection,
                                 int creatorId,
                                 int linkType,
                                 int debateId) {
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
    rel.set_id(linkId);
    if (debateId != 0) {
        rel.set_debate_id(debateId);
    }
    rel.add_from_claim_ids(fromClaimId);  // exactly one, until storage widens
    rel.set_to_claim_id(toClaimId);
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

}  // namespace RelationBuilder
