#include "DebateWrapper.h"
#include <iostream>
#include "Log.h"
#include "../../../src/gen/cpp/debate.pb.h"
#include "../../../src/gen/cpp/user.pb.h"

DebateWrapper::DebateWrapper(DebateDatabase& debateDatabase, StatementDatabase& statementDatabase, UserDatabase& userDatabase, DebateMembersDatabase& debateMembersDatabase, LinkDatabase& linkDatabase, ChallengeDatabase& challengeDatabase)
    : debateDb(debateDatabase),
      statementDb(statementDatabase),
      userDb(userDatabase),
      debateMembersDb(debateMembersDatabase),
      linkDb(linkDatabase),
      challengeDb(challengeDatabase) {}
std::vector<int> DebateWrapper::findChildrenIds(
    const int& parentId) {
    std::vector<int> childrenIds;
    debate::Claim parentClaim = findClaim(parentId);
    for (const auto& childId : parentClaim.proof().claim_ids()) {
        childrenIds.push_back(childId);
    }
    return childrenIds;
}

std::vector<std::pair<std::string,std::string>> DebateWrapper::findChildrenInfo(
    const int& parentId) {
    std::vector<std::pair<std::string,std::string>> childrenInfo;
    debate::Claim parentClaim = findClaim(parentId);

    for (const auto& claim_id: parentClaim.proof().claim_ids()) {
        debate::Claim claim = findClaim(claim_id);
        childrenInfo.emplace_back(std::to_string(claim.id()), claim.sentence());
    }
    return childrenInfo;
}

std::string DebateWrapper::findClaimSentence(
    const int& claimId) {
    debate::Claim claim = findClaim(claimId);
    return claim.sentence();
}

debate::Claim DebateWrapper::findClaim(const int& claimId) {
    std::vector<uint8_t> serializedData = statementDb.getStatementProtobuf(claimId);
    if (!serializedData.empty()) {
        debate::Claim claim;
        claim.ParseFromArray(serializedData.data(), serializedData.size());
        return claim;
    }
    // return an empty Claim if not found
    Log::warn("[DebateWrapper] Claim with ID " + std::to_string(claimId) + " not found in database.");
    return debate::Claim();
}

void DebateWrapper::initNewDebate(const std::string& topic, const int& creator_id) {
    debate::Debate debate;
    debate.set_topic(topic);
    debate.add_debater_ids(creator_id);
    debate.set_creator_id(creator_id);
    std::vector<uint8_t> serialized_debate(debate.ByteSizeLong());
    debate.SerializeToArray(serialized_debate.data(), serialized_debate.size());
    int newId = debateDb.addDebate(creator_id, topic, serialized_debate);
    if (newId == -1) {
        Log::error("[DebateWrapper] Failed to create new debate for topic: " + topic);
        return;
    }
    debate.set_id(newId);
    debate::Claim rootClaim;
    rootClaim.set_sentence(topic);
    rootClaim.set_parent_id(-1); // root claim has no parent
    rootClaim.set_creator_id(creator_id);
    addClaimToDB(rootClaim, creator_id, newId);
    debate.set_root_claim_id(rootClaim.id());
    std::vector<uint8_t> updatedSerializedDebate(debate.ByteSizeLong());
    debate.SerializeToArray(updatedSerializedDebate.data(), updatedSerializedDebate.size());
    debateDb.updateDebateProtobuf(debate.id(), creator_id, updatedSerializedDebate);
    debateMembersDb.addMember(newId, creator_id);
}

int DebateWrapper::initNewProofDebate(const std::string& challenge_sentence, const int& creator_id, const int& parent_challenge_id, debate::Debate& debateProto) {
    Log::debug("[DebateWrapper] Initializing new proof debate with challenge sentence: " + challenge_sentence);
    debateProto.set_topic(challenge_sentence);
    debateProto.add_debater_ids(creator_id);
    debateProto.set_creator_id(creator_id);
    debateProto.set_is_challenge(true);
    debateProto.set_parent_challenge_id(parent_challenge_id);
    std::vector<uint8_t> serialized_debate(debateProto.ByteSizeLong());
    debateProto.SerializeToArray(serialized_debate.data(), serialized_debate.size());
    int newId = debateDb.addDebate(creator_id, debateProto.topic(), serialized_debate);
    if (newId == -1) {
        Log::error("[DebateWrapper] Failed to create new debate for topic: " + challenge_sentence);
        return -1;
    }
    debateProto.set_id(newId);
        debate::Claim rootClaim;
    rootClaim.set_sentence(challenge_sentence);
    rootClaim.set_parent_id(-1); // root claim has no parent
    rootClaim.set_creator_id(creator_id);
    addClaimToDB(rootClaim, creator_id, newId);
    debateProto.set_root_claim_id(rootClaim.id());
    std::vector<uint8_t> updatedSerializedDebate(debateProto.ByteSizeLong());
    debateProto.SerializeToArray(updatedSerializedDebate.data(), updatedSerializedDebate.size());
    debateDb.updateDebateProtobuf(debateProto.id(), creator_id, updatedSerializedDebate);
    debateMembersDb.addMember(newId, creator_id);
    return newId;
}

debate::Claim DebateWrapper::findClaimParent(const int& claimId) {
    debate::Claim claim = findClaim(claimId);
    int parentId = claim.parent_id();
    if (parentId == -1){
        return claim; // root claim has no parent
    }
    return findClaim(parentId);
}

void DebateWrapper::addClaimUnderParent(
    const int& parentId, 
    const std::string& claimText, 
    const std::string& description,
    const int& user_id,
    const int& debate_id) {

    debate::Claim parentClaimFromDB = findClaim(parentId);

    // Add new claim
    debate::Claim childClaim;
    childClaim.set_sentence(claimText);
    childClaim.set_parent_id(parentId);
    childClaim.set_description(description);
    childClaim.set_creator_id(user_id);
    childClaim.set_debate_id(debate_id);
    addClaimToDB(childClaim, user_id, debate_id);
    parentClaimFromDB.mutable_proof()->add_claim_ids(childClaim.id());
    updateClaimInDB(parentClaimFromDB);
}

void DebateWrapper::addClaimToDB(debate::Claim& claim, const int& user_id, const int& debate_id) {
    claim.set_creator_id(user_id);
    claim.set_debate_id(debate_id);
    std::vector<uint8_t> serializedData(claim.ByteSizeLong());
    claim.SerializeToArray(serializedData.data(), serializedData.size());
    int newId = statementDb.addStatement(
        -1, 
        claim.sentence(), 
        serializedData,
        user_id
    );
    claim.set_id(newId);
    std::vector<uint8_t> updatedSerializedData(claim.ByteSizeLong());
    claim.SerializeToArray(updatedSerializedData.data(), updatedSerializedData.size());
    statementDb.updateStatementProtobuf(
        claim.id(), 
        updatedSerializedData
    );
    statementDb.updateStatementRoot(
        claim.id(), 
        claim.id()
    );
}

void DebateWrapper::updateClaimInDB(const debate::Claim& claim) {
    std::vector<uint8_t> serializedData(claim.ByteSizeLong());
    claim.SerializeToArray(serializedData.data(), serializedData.size());
    statementDb.updateStatementProtobuf(
        claim.id(), 
        serializedData
    );
}

void DebateWrapper::deleteDebate(const int& debateId, const int& user_id) {
    // find the debate protobuf
    std::vector<uint8_t> debateData = debateDb.getDebateProtobuf(debateId);
    if (debateData.empty()) {
        Log::error("[DebateWrapper] Debate with ID " + std::to_string(debateId) + " not found.");
    }
    else {
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateData.data(), debateData.size());
        
        // find all debatemembers and delete
        debateMembersDb.removeAllMembersFromDebate(debateProto.root_claim_id());
        // delete all claims associated with the debate
        debate::Claim rootClaim = findClaim(debateProto.root_claim_id());
        std::vector<int> claimsToDelete;
        claimsToDelete.push_back(debateProto.root_claim_id());

        size_t index = 0;
        while (index < claimsToDelete.size()) { // BFS-like traversal
            int currentClaimId = claimsToDelete[index];
            debate::Claim currentClaim = findClaim(currentClaimId);
            for (const auto& childId : currentClaim.proof().claim_ids()) {
                claimsToDelete.push_back(childId);
            }
            index++;
        }

        for (const auto& claimId : claimsToDelete) {
            deleteClaim(claimId);
        }

        // Delete the debate entry from the DEBATE table
        // debateDb.removeDebate(std::stoi(debateId), user);
        debateMembersDb.removeMember(debateId, user_id);
    }
}

void DebateWrapper::deleteClaim(const int& claimId) {
    // find the claim parent to remove it's reference and also find it's children to delete them recursively
    debate::Claim claim = findClaim(claimId);
    int parentId = claim.parent_id();
    if (parentId != -1) {
        debate::Claim parentClaim = findClaim(parentId);
        auto* proof = parentClaim.mutable_proof();
        auto& claimIds = *proof->mutable_claim_ids();
        claimIds.erase(
            std::remove(claimIds.begin(), claimIds.end(), claimId),
            claimIds.end()
        );
        updateClaimInDB(parentClaim);
    }
    // delete all children recursively
    for (const auto& childId : claim.proof().claim_ids()) {
        deleteClaim(childId);
    }
    // delete all links in the proof
    // for (const auto& linkId : claim.proof().link_ids()) {
    //     deleteLinkById(std::stoi(linkId));
    // }
    // delete links that go to and from this claim
    std::vector<int> linksToDelete = findLinksUnder(claimId);
    auto linksFromThisClaim = linkDb.getLinksFromClaim(claimId);
    auto linksToThisClaim =  linkDb.getLinksToClaim(claimId);
    for (const auto& linkTuple : linksFromThisClaim) {
        int linkId = std::get<0>(linkTuple);
        linksToDelete.push_back(linkId);
    }
    for (const auto& linkTuple : linksToThisClaim) {
        int linkId = std::get<0>(linkTuple);
        linksToDelete.push_back(linkId);
    }
    for (const auto& linkId : linksToDelete) {
        deleteLinkById(linkId);
    }

    statementDb.deleteStatement(claimId);
}

void DebateWrapper::deleteAllDebates(const int& user_id) {
    std::vector<int> allDebates = debateMembersDb.getDebateIdsForUser(user_id);
    for (const auto& debateId : allDebates) {
        deleteDebate(debateId, user_id);
    }
}

void DebateWrapper::moveUserToClaim(const int& user_id, const int& claimId) {
    user::User userProto = getUserProtobuf(user_id);

    userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_DEBATING);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_root_claim()->set_id(claimId);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(claimId);
    
    std::vector<uint8_t> updatedData(userProto.ByteSizeLong());
    userProto.SerializeToArray(updatedData.data(), updatedData.size());
    
    bool success = userDb.updateUserProtobuf(user_id, updatedData);
    
    if (success) {
        Log::debug("[DebateWrapper] Successfully moved user " + std::to_string(user_id) + " to claim id " + std::to_string(claimId) 
                    + " (state=DEBATING)");
    } else {
        Log::error("[DebateWrapper][ERR] Failed to update user protobuf for " + std::to_string(user_id));
    }
    
    return;
}

std::vector<int> DebateWrapper::getUserDebateIds(const int& user_id) {
    return debateMembersDb.getDebateIdsForUser(user_id);
}

std::vector<uint8_t> DebateWrapper::getDebateProtobuf(const int& debateId) {
    return debateDb.getDebateProtobuf(debateId);
}

std::vector<uint8_t> DebateWrapper::getUserProtobufBinary(const int& user_id) {
    return userDb.getUserProtobuf(user_id);
}

user::User DebateWrapper::getUserProtobuf(const int& user_id) {
    std::vector<uint8_t> userData = userDb.getUserProtobuf(user_id);
    user::User userProto;
    if (!userProto.ParseFromArray(userData.data(), static_cast<int>(userData.size()))) {
        Log::error("[DebateWrapper][ERR] Failed to parse user protobuf for " + std::to_string(user_id));
    }
    return userProto;
}

void DebateWrapper::updateUserProtobuf(const int& user_id, const user::User& userProto) {
    std::vector<uint8_t> protobufData(userProto.ByteSizeLong());
    userProto.SerializeToArray(protobufData.data(), protobufData.size());
    bool success = userDb.updateUserProtobuf(user_id, protobufData);
    if (success) {
        Log::debug("[DebateWrapper] Successfully updated user protobuf for " + std::to_string(user_id));
    } else {
        Log::error("[DebateWrapper][ERR] Failed to update user protobuf for " + std::to_string(user_id));
    }
}

void DebateWrapper::updateUserProtobufBinary(const int& user_id, const std::vector<uint8_t>& protobufData) {
    bool success = userDb.updateUserProtobuf(user_id, protobufData);
    if (success) {
        Log::debug("[DebateWrapper] Successfully updated user protobuf for " + std::to_string(user_id));
    } else {
        Log::error("[DebateWrapper][ERR] Failed to update user protobuf for " + std::to_string(user_id));
    }
}

void DebateWrapper::changeDescriptionOfClaim(
    const int& claimId,
    const std::string& newDescription) {
    debate::Claim claim = findClaim(claimId);
    claim.set_description(newDescription);
    updateClaimInDB(claim);
}

void DebateWrapper::editClaimText(
    const int& claimId,
    const std::string& newText) {
    debate::Claim claim = findClaim(claimId);
    claim.set_sentence(newText);
    updateClaimInDB(claim);
    // also update the actual topic in the statement database
    statementDb.updateStatementContent(claimId, newText);
}

int DebateWrapper::addLink(int fromClaimId, int toClaimId, const std::string& connection, int creator_id) {
    int linkId = linkDb.addLink(fromClaimId, toClaimId, connection, creator_id);
    Log::debug("[DebateWrapper] Added link from claim " + std::to_string(fromClaimId) + " to claim " + std::to_string(toClaimId) + " by user " + std::to_string(creator_id));
    return linkId;
}

std::vector<int> DebateWrapper::findLinksUnder(const int& claimId) {
    // should be in the proof of the claim
    debate::Claim claim = findClaim(claimId);
    std::vector<int> linkIds;
    for (const auto& linkId : claim.proof().link_ids()) {
        linkIds.push_back((linkId));
        Log::debug("[IMPROTATN] Found link ID: " + std::to_string(linkId) + " under Claim ID: " + std::to_string(claimId));
    }
    return linkIds;
}

debate::Link DebateWrapper::getLinkById(int linkId) {
    debate::Link linkProto;
    auto linkData = linkDb.getLinkById(linkId);
    
    if (linkData.has_value()) {
        const auto& link = linkData.value();
        linkProto.set_connect_from(std::get<1>(link));
        linkProto.set_connect_to(std::get<2>(link));
        linkProto.set_connection(std::get<3>(link));
        // linkProto.set_creator(std::get<4>(link));
        
        Log::debug("[DebateWrapper] Retrieved link ID: " + std::to_string(std::get<0>(link))
            + " from Claim ID: " + std::to_string(std::get<1>(link))
            + " to Claim ID: " + std::to_string(std::get<2>(link))
            + " with connection: " + std::get<3>(link)
            + " created by: " + std::to_string(std::get<4>(link)));
    } else {
        Log::warn("[DebateWrapper] Link with ID " + std::to_string(linkId) + " not found.");
    }
    
    return linkProto;
}

void DebateWrapper::deleteLinkById(int linkId) {
    bool success = linkDb.deleteLink(linkId);
    if (success) {
        Log::debug("[DebateWrapper] Successfully deleted link with ID " + std::to_string(linkId));
    } else {
        Log::error("[DebateWrapper][ERR] Failed to delete link with ID " + std::to_string(linkId));
    }
}

void DebateWrapper::addMemberToDebate(const int& debateId, const int& user_id) {
    // check if user already a member
    if (debateMembersDb.isMember(debateId, user_id)) {
        Log::info("[DebateWrapper] User " + std::to_string(user_id) + " is already a member of debate " + std::to_string(debateId));
        return;
    }
    // check if debate exists
    if (debateDb.debateExists(debateId) == false) {
        Log::error("[DebateWrapper][ERR] Debate with ID " + std::to_string(debateId) + " does not exist. Cannot add member.");
        return;
    }
    debateMembersDb.addMember(debateId, user_id);
}

int DebateWrapper::addChallenge(
    const int& creator_id,
    const int& challenged_claim_id,
    debate::Challenge challengeProtobuf
) {
    // serialize challengeProtobuf
    std::vector<uint8_t> challengeProtobufData(challengeProtobuf.ByteSizeLong());
    challengeProtobuf.SerializeToArray(challengeProtobufData.data(), challengeProtobufData.size());
    int challenge_id = challengeDb.addChallenge(
        challengeProtobufData,
        creator_id,
        challenged_claim_id
    );
    return challenge_id;
}

std::vector<int> DebateWrapper::getChallengesAgainstClaim(const int& claimId) {
    return challengeDb.getChallengesAgainstClaim(claimId);
}

debate::Challenge DebateWrapper::getChallengeProtobuf(int challengeId) {
    debate::Challenge challengeProto;
    std::vector<uint8_t> challengeData = challengeDb.getChallengeProtobuf(challengeId);
    if (!challengeData.empty()) {
        if (!challengeProto.ParseFromArray(challengeData.data(), static_cast<int>(challengeData.size()))) {
            Log::error("[DebateWrapper][ERR] Failed to parse challenge protobuf for challenge ID " + std::to_string(challengeId));
        }
    } else {
        Log::error("[DebateWrapper][ERR] Challenge with ID " + std::to_string(challengeId) + " not found.");
    }
    return challengeProto;
}

void DebateWrapper::deleteChallenge(const int& challengeId) {
    bool success = challengeDb.deleteChallenge(challengeId);
    if (success) {
        Log::debug("[DebateWrapper] Successfully deleted challenge with ID " + std::to_string(challengeId));
    } else {
        Log::error("[DebateWrapper][ERR] Failed to delete challenge with ID " + std::to_string(challengeId));
    }
}

void DebateWrapper::updateDebateProtobuf(const int& debateId, const debate::Debate& debateProto) {
    std::vector<uint8_t> protobufData(debateProto.ByteSizeLong());
    debateProto.SerializeToArray(protobufData.data(), protobufData.size());
    debateDb.updateDebateProtobuf(debateId, debateProto.creator_id(), protobufData);
}

int DebateWrapper::findDebateId(const int& claimId) {
    debate::Claim claim = findClaim(claimId);
    int debate_id = claim.debate_id();
    return debate_id;
}