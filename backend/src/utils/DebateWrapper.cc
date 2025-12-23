#include "DebateWrapper.h"


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
    debate::Claim* parentClaim = nullptr;
    for (int i = 0; i < debateProto.claims_size(); ++i) {
        if (debateProto.claims(i).id() == parentId) {
            parentClaim = debateProto.mutable_claims(i);
            break;
        }
    }
    
    if (parentClaim) {
        parentClaim->add_children_ids(std::to_string(debateProto.num_items()));
    }
    
    // Add new claim
    debate::Claim* newClaim = debateProto.add_claims();
    newClaim->set_sentence(claimText);
    newClaim->set_connection_to_parent(connectionToParent);
    newClaim->set_parent_id(parentId);
    newClaim->set_id(std::to_string(debateProto.num_items()));
    debateProto.set_num_items(debateProto.num_items() + 1);
}

void DebateWrapper::setDebateTopic(
    const std::string& newTopic) {
    debateProto.set_topic(newTopic);
}