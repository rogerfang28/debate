#pragma once

#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../database/handlers/StatementDatabaseHandler.h"
#include "./pathUtils.h"
#include <vector>
#include <string>

class DebateWrapper {
public:
    explicit DebateWrapper(debate::Debate& debate); // so this should pass by reference right so we can modify it and changes will be saved
    // DebateWrapper() = default;
    std::vector<debate::Claim> findChildren(const std::string& parentId);
    debate::Claim findClaim(const std::string& claimId);
    void initNewDebate(const std::string& topic, const std::string& owner);
    debate::Claim findClaimParent(const std::string& claimId);
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
    std::string getTopic() const;
    std::string findClaimSentence(
        const std::string& claimId);
    std::vector<std::string> findChildrenIds(
        const std::string& parentId);
    std::vector<std::pair<std::string,std::string>> findChildrenInfo(
        const std::string& parentId); // returns vector of (id, sentence) pairs

private:
    debate::Debate& debateProto;
    debate::Claim* findClaimProto(const std::string& claimId);
    StatementDatabaseHandler statementDBHandler{utils::getDatabasePath()}; // path from path utils
    void addClaimToDB(debate::Claim& claim);
    void updateClaimInDB(const debate::Claim& claim);
};