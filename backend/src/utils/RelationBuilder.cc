#include "RelationBuilder.h"

namespace RelationBuilder {

debate::Relationship fromLinkRow(int linkId,
                                 int fromClaimId,
                                 int toClaimId,
                                 const std::string& connection,
                                 int creatorId,
                                 int linkType,
                                 int debateId) {
    debate::Relationship rel;

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

    switch (static_cast<debate::LinkType>(linkType)) {
        case debate::LinkType::CHALLENGE:
            rel.set_role(debate::OPPOSES);
            break;
        case debate::LinkType::PARENT_CHILD:
        case debate::LinkType::NORMAL:
        default:
            // NORMAL carries no agreed meaning in the old model and nothing
            // computes from it, so folding it in with PARENT_CHILD is the
            // reading that changes no behaviour.
            rel.set_role(debate::SUPPORTS);
            break;
    }

    return rel;
}

}  // namespace RelationBuilder
