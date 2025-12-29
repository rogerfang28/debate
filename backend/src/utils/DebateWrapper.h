#pragma once

#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "./pathUtils.h"
#include <vector>
#include <string>
#include "../database/handlers/DebateDatabase.h"
#include "../database/handlers/UserDatabase.h"
#include "../database/handlers/StatementDatabase.h"
#include "../database/handlers/DebateMembersDatabase.h"
#include "../database/handlers/LinkDatabase.h"

class DebateWrapper {
public:
    explicit DebateWrapper(DebateDatabase& debateDatabase, StatementDatabase& statementDatabase, UserDatabase& userDatabase, DebateMembersDatabase& debateMembersDatabase, LinkDatabase& linkDatabase);
    // DebateWrapper() = default;
    std::vector<debate::Claim> findChildren(const std::string& parentId);
    debate::Claim findClaim(const std::string& claimId);
    void initNewDebate(const std::string& topic, const int& owner_id);
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
    void deleteDebate(const std::string& debateId, const int& user_id);
    void deleteClaim(const std::string& claimId);
    void deleteAllDebates(const int& user_id);
    void moveUserToClaim(const int& user_id, const std::string& claimId);
    std::vector<std::string> getUserDebateIds(const int& user_id);
    std::vector<uint8_t> getDebateProtobuf(const std::string& debateId);
    // std::vector<uint8_t> getUserProtobufBinary(const std::string& user_id);
    user::User getUserProtobuf(const int& user_id);
    std::vector<uint8_t> getUserProtobufBinary(const int& user_id);

    void updateUserProtobufBinary(const int& user_id, const std::vector<uint8_t>& protobufData);
    void updateUserProtobuf(const int& user_id, const user::User& userProto);

    int addLink(std::string fromClaimId, std::string toClaimId, const std::string& connection, int creator_id);
    std::vector<int> findLinksUnder(const std::string& claimId);
    debate::Link getLinkById(int linkId);
    void updateClaimInDB(const debate::Claim& claim);
    void deleteLinkById(int linkId);
    void addMemberToDebate(const std::string& debateId, const int& user_id);

private:
    debate::Claim* findClaimProto(const std::string& claimId);
    DebateDatabase& debateDb;
    StatementDatabase& statementDb;
    UserDatabase& userDb;
    DebateMembersDatabase& debateMembersDb;
    LinkDatabase& linkDb;
    void addClaimToDB(debate::Claim& claim);
    // void updateClaimInDB(const debate::Claim& claim);
};