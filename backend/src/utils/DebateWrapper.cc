#include "DebateWrapper.h"
#include "../database/handlers/StatementDatabaseHandler.h"

DebateWrapper::DebateWrapper(debate::Debate& debate)
    : debateProto(debate) {}

std::vector<debate::Claim> DebateWrapper::findChildren(
    const std::string& parentId) const {
    std::vector<debate::Claim> children;
    for (const auto& claim : debateProto.claims()) {
        if (claim.parent_id() == parentId) {
            children.push_back(claim);
        }
    }
    return children;
}

std::vector<std::string> DebateWrapper::findChildrenIds(
    const std::string& parentId) const {
    std::vector<std::string> childrenIds;
    for (const auto& claim : debateProto.claims()) {
        if (claim.parent_id() == parentId) {
            childrenIds.push_back(claim.id());
        }
    }
    return childrenIds;
}

std::vector<std::pair<std::string,std::string>> DebateWrapper::findChildrenInfo(
    const std::string& parentId) const {
    std::vector<std::pair<std::string,std::string>> childrenInfo;
    for (const auto& claim : debateProto.claims()) {
        if (claim.parent_id() == parentId) {
            childrenInfo.emplace_back(claim.id(), claim.sentence());
        }
    }
    return childrenInfo;
}

std::string DebateWrapper::getTopic() const {
    return debateProto.topic();
}

std::string DebateWrapper::findClaimSentence(
    const std::string& claimId) const {
    for (const auto& claim : debateProto.claims()) {
        if (claim.id() == claimId) {
            return claim.sentence();
        }
    }
    return "";
}

debate::Claim DebateWrapper::findClaim(const std::string& claimId) const {
    for (const auto& claim : debateProto.claims()) {
        if (claim.id() == claimId) {
            return claim;
        }
    }
    // return an empty Claim if not found
    return debate::Claim();
}

void DebateWrapper::initNewDebate(const std::string& topic, const std::string& owner) {
    // assumes empty debateProto
    debateProto.set_topic(topic);
    debateProto.set_owner(owner);
    debateProto.set_num_items(0);
    // debate::Claim rootClaim;
    // rootClaim.set_sentence(topic);
    // int newId = statementDBHandler.addStatement(
    //     rootClaim.id(), 
    //     rootClaim.sentence(), 
    //     std::vector<uint8_t>{} // empty protobuf data for now
    // );
    // rootClaim.set_id(std::to_string(newId));
    // statementDBHandler.updateStatementProtobuf(
    //     rootClaim.id(), 
    //     std::vector<uint8_t>(rootClaim.SerializeAsString().begin(), rootClaim.SerializeAsString().end())
    // );

}



debate::Claim DebateWrapper::findClaimParent(const std::string& claimId) const {
    debate::Claim claim = findClaim(claimId);
    std::string parentId = claim.parent_id();
    return findClaim(parentId);
}

void DebateWrapper::addClaimUnderParent(
    const std::string& parentId, 
    const std::string& claimText, 
    const std::string& connectionToParent) {
    // Find and modify parent claim using pointer
    // debate::Claim* parentClaim = findClaim(parentId);
    debate::Claim* parentClaim = nullptr;
    for (int i = 0; i < debateProto.claims_size(); ++i) {
        if (debateProto.claims(i).id() == parentId) {
            parentClaim = debateProto.mutable_claims(i);
            break;
        }
    }
    
    // Add new claim
    debate::Claim* newClaim = debateProto.add_claims();
    newClaim->set_sentence(claimText);
    newClaim->set_connection_to_parent(connectionToParent);
    newClaim->set_parent_id(parentId);
    newClaim->set_id(std::to_string(debateProto.num_items()));
    debateProto.set_num_items(debateProto.num_items() + 1);

    if (parentClaim) {
        parentClaim->mutable_proof()->add_claim_ids(newClaim->id());
        parentClaim->add_children_ids(newClaim->id());
    }
}

void DebateWrapper::setDebateTopic(
    const std::string& newTopic) {
    debateProto.set_topic(newTopic);
}