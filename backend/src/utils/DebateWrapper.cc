#include "DebateWrapper.h"


DebateWrapper::DebateWrapper(const debate::Debate& debate) 
    : debate(debate) {}

std::vector<debate::Claim> DebateWrapper::findChildren(
    const std::string& parentId) const {
    std::vector<debate::Claim> children;
    for (const auto& claim : debate.claims()) {
        if (claim.parent_id() == parentId) {
            children.push_back(claim);
        }
    }
    return children;
}

debate::Claim DebateWrapper::findClaim(const std::string& debate_id, const std::string& claimId) const {
    for (const auto& claim : debate.claims()) {
        if (claim.id() == claimId) {
            return claim;
        }
    }
    // return an empty Claim if not found
    return debate::Claim();
}

void createNewDebate(const std::string& topic, const std::string& owner) {
    
}

void findStatementParent(const std::string& statementId) {
    
}

void addStatementUnderParent(
    const std::string& parentId, 
    const std::string& statementText, 
    const std::string& connectionToParent) {
    
}