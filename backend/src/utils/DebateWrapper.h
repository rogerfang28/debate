#pragma once

#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../database/handlers/StatementDatabaseHandler.h"
#include "../database/handlers/DebateMembersDatabaseHandler.h"
#include "../database/handlers/DebateDatabaseHandler.h"
#include "./pathUtils.h"
#include <vector>
#include <string>

class DebateWrapper {
public:
    explicit DebateWrapper();
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
    std::string findClaimSentence(
        const std::string& claimId);
    std::vector<std::string> findChildrenIds(
        const std::string& parentId);
    std::vector<std::pair<std::string,std::string>> findChildrenInfo(
        const std::string& parentId); // returns vector of (id, sentence) pairs
    void deleteDebate(const std::string& debateId);
    void deleteClaim(const std::string& claimId);
    void deleteAllDebates(const std::string& user);

private:
    debate::Claim* findClaimProto(const std::string& claimId);
    StatementDatabaseHandler statementDBHandler{utils::getDatabasePath()}; // path from path utils
    DebateDatabaseHandler debateDBHandler{utils::getDatabasePath()};
    DebateMembersDatabaseHandler debateMembersDBHandler{utils::getDatabasePath()};
    void addClaimToDB(debate::Claim& claim);
    void updateClaimInDB(const debate::Claim& claim);
};