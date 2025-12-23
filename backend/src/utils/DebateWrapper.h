#pragma once

#include "../../../../../src/gen/cpp/debate.pb.h"
#include <vector>
#include <string>

class DebateWrapper {
public:
    explicit DebateWrapper(debate::Debate& debate); // so this should pass by reference right so we can modify it and changes will be saved
    std::vector<debate::Claim> findChildren(const std::string& parentId) const;
    debate::Claim findClaim(const std::string& claimId) const;
    void initNewDebate(const std::string& topic, const std::string& owner);
    debate::Claim findClaimParent(const std::string& claimId) const;
    void addClaimUnderParent(
        const std::string& parentId, 
        const std::string& claimText, 
        const std::string& connectionToParent);
    void changeDescriptionOfClaim(
        const std::string& claimId,
        const std::string& newDescription);
    void editClaimText(
        const std::string& claimId,
        const std::string& newText);
    void setDebateTopic(
        const std::string& newTopic);
    

private:
    debate::Debate& debateProto;
};