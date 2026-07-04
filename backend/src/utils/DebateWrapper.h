#pragma once

#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "./pathUtils.h"
#include <vector>
#include <string>
#include <tuple>
#include <map>
#include <set>
#include "../database/debate/DatabaseWrapper.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"

class DebateWrapper {
public:
    explicit DebateWrapper(DatabaseWrapper& databaseWrapper);
    std::vector<debate::Claim> findChildren(const std::string& parentId);
    debate::Claim getClaimById(const int& claimId);
    void initNewDebate(const std::string& topic, const int& owner_id);
    debate::Claim findClaimParent(const int& claimId);
    bool isRoot(const int& claimId);
    int createClaim(
        const std::string& claimText,
        const std::string& description,
        const int& user_id,
        const int& debate_id);
    int addClaimUnderParent(
        const int& parentId, 
        const std::string& claimText, 
        const std::string& connectionToParent,
        const int& user_id,
        const int& debate_id);
    void changeDescriptionOfClaim(
        const int& claimId,
        const std::string& newDescription);
    void editClaimText(
        const int& claimId,
        const std::string& newText);
    std::string findClaimSentence(
        const int& claimId);
    std::vector<int> findChildrenIds(
        const int& parentId);
    std::vector<std::pair<std::string,std::string>> findChildrenInfo(
        const int& parentId); // returns vector of (id, sentence) pairs
    void deleteDebate(const int& debateId, const int& user_id);
    void deleteClaim(const int& claimId);
    void deleteAllDebates(const int& user_id);
    void moveUserToClaim(const int& user_id, const int& claimId);
    std::vector<int> getUserDebateIds(const int& user_id);
    std::vector<uint8_t> getDebateProtobuf(const int& debateId);
    user::User getUserProtobuf(const int& user_id);
    std::vector<uint8_t> getUserProtobufBinary(const int& user_id);

    void updateUserProtobufBinary(const int& user_id, const std::vector<uint8_t>& protobufData);
    void updateUserProtobuf(const int& user_id, const user::User& userProto);

    int addLink(int fromClaimId, int toClaimId, const std::string& connection, int creator_id, int debate_id, debate::LinkType link_type = debate::LinkType::NORMAL);
    std::vector<std::vector<uint8_t>> getStatementsForDebate(const int& debate_id);
    std::vector<std::vector<uint8_t>> getStatementsForDebateAndCreators(const int& debate_id, const std::vector<int>& creator_ids);
    std::vector<std::tuple<int, int, int, std::string, int, int>> getLinksForDebate(const int& debate_id);
    std::vector<std::tuple<int, int, int, std::string, int, int>> getLinksForDebateAndCreators(const int& debate_id, const std::vector<int>& creator_ids);
    std::vector<int> findLinksUnder(const int& claimId);
    debate::Relationship getLinkById(int linkId);
    void updateClaimInDB(const debate::Claim& claim);
    void deleteLinkById(int linkId);
    void addMemberToDebate(const int& debateId, const int& user_id);
    void updateDebateProtobuf(const int& debateId, const debate::Debate& debateProto);
    int findDebateId(const int& claimId);
    user_engagement::DebateList FillUserDebateList(const int& user_id);

    void SaveVersionOfClaim(const int& claim_id);
    void RestorePreviousVersionOfClaim(const int& claim_id);
    void UpdateStatusOfAllClaimsInDebate(const int& debate_id);
    void PropagateClaimStatuses(const int& debate_id, const std::set<int>& conceded_claims = {});
    std::vector<int> findUsersInDebate(const int& debate_id);

private:
    DatabaseWrapper& databaseWrapper;
    void addClaimToDB(debate::Claim& claim, const int& user_id, const int& debate_id);
    // void updateClaimInDB(const debate::Claim& claim);

    // Determines what a claim's status should become for one user, given its
    // current status and its PARENT_CHILD/CHALLENGE neighbors. Pure computation —
    // does not write anything; callers decide whether/how to apply the result.
    debate::ClaimStatus ComputeStatusForUser(
        const int& claimId,
        const std::string& user,
        const debate::ClaimStatus& currentStatus,
        const std::map<int, std::vector<int>>& childrenMap,
        const std::map<int, std::vector<int>>& challengeIncoming,
        const std::set<int>& conceded_claims);
};