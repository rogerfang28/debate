#pragma once

#include "debate.pb.h"
#include "user.pb.h"
#include "./pathUtils.h"
#include <vector>
#include <string>
#include <tuple>
#include <map>
#include <set>
#include "../database/debate/DatabaseWrapper.h"
#include "../database/debate/ChatDatabase.h"
#include "user_engagement.pb.h"

class DebateWrapper {
public:
    explicit DebateWrapper(DatabaseWrapper& databaseWrapper);
    std::vector<debate::Claim> findChildren(const std::string& parentId);
    debate::Claim getClaimById(const int& claimId);
    // `voice_id` and `source` are the root claim's, same meaning as on
    // createClaim: whose claim it is, and where it was taken from.
    void initNewDebate(const std::string& topic, const int& owner_id, int voice_id = 0,
                       const debate::Source* source = nullptr);
    debate::Claim findClaimParent(const int& claimId);
    bool isRoot(const int& claimId);
    // `voice_id` is WHOSE claim it is when that is not `user_id` (see
    // ClaimVoice.h); 0 means the entrant speaks for themselves. Truth is
    // seeded for the voice. `source`, if given and not blank, is recorded
    // on the claim as where it was taken from.
    int createClaim(
        const std::string& claimText,
        const std::string& description,
        const int& user_id,
        const int& debate_id,
        int voice_id = 0,
        const debate::Source* source = nullptr);
    // `basis` is stored as the link's connection -- the field that until now
    // held a hardcoded constant. Empty falls back to that old constant so
    // existing callers keep their previous behaviour.
    int addClaimUnderParent(
        const int& parentId,
        const std::string& claimText,
        const std::string& connectionToParent,
        const int& user_id,
        const int& debate_id,
        const std::string& basis = "",
        int relation_id = 0,
        int* out_relation_id = nullptr,
        int voice_id = 0,
        const debate::Source* source = nullptr);
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

    // `relation_id` groups rows that are ONE relation with several
    // from-claims. 0 means "its own relation".
    // `to_link_id` > 0 aims a CHALLENGE row at an argument (that relation id)
    // instead of at toClaimId; see LinkDatabase::addLink.
    int addLink(int fromClaimId, int toClaimId, const std::string& connection, int creator_id, int debate_id, debate::LinkType link_type = debate::LinkType::NORMAL, int relation_id = 0, int to_link_id = 0);
    // Row id -> relation id, for every row in the debate that attacks a relation.
    std::map<int, int> getRelationTargets(const int& debate_id);
    std::vector<std::vector<uint8_t>> getStatementsForDebate(const int& debate_id);
    std::vector<std::vector<uint8_t>> getStatementsForDebateAndCreators(const int& debate_id, const std::vector<int>& creator_ids);
    std::vector<std::tuple<int, int, int, std::string, int, int, int>> getLinksForDebate(const int& debate_id);
    std::vector<std::tuple<int, int, int, std::string, int, int, int>> getLinksForDebateAndCreators(const int& debate_id, const std::vector<int>& creator_ids);
    std::vector<int> findLinksUnder(const int& claimId);
    debate::Relationship getLinkById(int linkId);
    // Scans fromClaimId's link_ids for an outgoing CHALLENGE link (a link where
    // this claim is the challenger, i.e. connect_from() == fromClaimId).
    // Returns a default (empty) Relationship if none found.
    debate::Relationship findOutgoingChallengeLink(int fromClaimId);
    void updateClaimInDB(const debate::Claim& claim);
    void deleteLinkById(int linkId);
    void addMemberToDebate(const int& debateId, const int& user_id);
    void updateDebateProtobuf(const int& debateId, const debate::Debate& debateProto);
    int findDebateId(const int& claimId);
    user_engagement::DebateList FillUserDebateList(const int& user_id);

    // Read-only view of the move log. Used by the shadow comparison only --
    // nothing computes or renders from it yet.
    std::vector<debate::Move> getMovesForDebate(const int& debate_id);

    // The AI chat: one user's conversation about one debate; see ChatHandler.
    void appendChatMessage(const int& user_id, const int& debate_id, const std::string& role,
                           const std::string& text);
    std::vector<ChatDatabase::Message> chatTranscript(const int& user_id, const int& debate_id);
    void clearChat(const int& user_id, const int& debate_id);

    void SaveVersionOfClaim(const int& claim_id);
    void RestorePreviousVersionOfClaim(const int& claim_id);
    std::vector<int> findUsersInDebate(const int& debate_id);

private:
    DatabaseWrapper& databaseWrapper;
    void addClaimToDB(debate::Claim& claim, const int& user_id, const int& debate_id);
    // void updateClaimInDB(const debate::Claim& claim);
};