#pragma once

#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "./pathUtils.h"
#include <vector>
#include <string>
#include "../database/handlers/DatabaseWrapper.h"

class DebateWrapper {
public:
    explicit DebateWrapper(DatabaseWrapper& databaseWrapper);
    std::vector<debate::Claim> findChildren(const std::string& parentId);
    debate::Claim getClaimById(const int& claimId);
    void initNewDebate(const std::string& topic, const int& owner_id);
    debate::Claim findClaimParent(const int& claimId);
    void addClaimUnderParent(
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

    int addLink(int fromClaimId, int toClaimId, const std::string& connection, int creator_id);
    std::vector<int> findLinksUnder(const int& claimId);
    debate::Link getLinkById(int linkId);
    void updateClaimInDB(const debate::Claim& claim);
    void deleteLinkById(int linkId);
    void addMemberToDebate(const int& debateId, const int& user_id);
    int addChallenge(
        const int& creator_id,
        const int& challenged_claim_id,
        debate::Challenge challengeProtobuf);
    void deleteChallenge(const int& challengeId);
    std::vector<int> getChallengesAgainstClaim(const int& claimId);
    debate::Challenge getChallengeProtobuf(int challengeId);
    int initNewProofDebate(
        const std::string& challengeSentence,
        const int& creator_id,
        const int& parent_challenge_id,
        debate::Debate& debateProto);
    void updateDebateProtobuf(const int& debateId, const debate::Debate& debateProto);
    int findDebateId(const int& claimId);

private:
    DatabaseWrapper& databaseWrapper;
    void addClaimToDB(debate::Claim& claim, const int& user_id, const int& debate_id);
    // void updateClaimInDB(const debate::Claim& claim);
};