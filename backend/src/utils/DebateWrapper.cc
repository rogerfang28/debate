#include "DebateWrapper.h"
#include <iostream>
#include "Log.h"
#include "../../../src/gen/cpp/debate.pb.h"
#include "../../../src/gen/cpp/user.pb.h"

DebateWrapper::DebateWrapper(DatabaseWrapper& dbWrapper)
    : databaseWrapper(dbWrapper) {}
std::vector<int> DebateWrapper::findChildrenIds(
    const int& parentId) {
    std::vector<int> childrenIds;
    debate::Claim parentClaim = getClaimById(parentId);
    for (const auto& childId : parentClaim.proof().claim_ids()) {
        childrenIds.push_back(childId);
    }
    return childrenIds;
}

std::vector<std::pair<std::string,std::string>> DebateWrapper::findChildrenInfo(
    const int& parentId) {
    std::vector<std::pair<std::string,std::string>> childrenInfo;
    debate::Claim parentClaim = getClaimById(parentId);

    for (const auto& claim_id: parentClaim.proof().claim_ids()) {
        debate::Claim claim = getClaimById(claim_id);
        childrenInfo.emplace_back(std::to_string(claim.id()), claim.sentence());
    }
    return childrenInfo;
}

std::string DebateWrapper::findClaimSentence(
    const int& claimId) {
    debate::Claim claim = getClaimById(claimId);
    return claim.sentence();
}

debate::Claim DebateWrapper::getClaimById(const int& claimId) {
    std::vector<uint8_t> serializedData = databaseWrapper.statements.getStatementProtobuf(claimId);
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
    int newId = databaseWrapper.debates.addDebate(creator_id, topic, serialized_debate);
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
    databaseWrapper.debates.updateDebateProtobuf(debate.id(), creator_id, updatedSerializedDebate);
    databaseWrapper.debateMembers.addMember(newId, creator_id);
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
    int newId = databaseWrapper.debates.addDebate(creator_id, debateProto.topic(), serialized_debate);
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
    databaseWrapper.debates.updateDebateProtobuf(debateProto.id(), creator_id, updatedSerializedDebate);
    databaseWrapper.debateMembers.addMember(newId, creator_id);
    return newId;
}

debate::Claim DebateWrapper::findClaimParent(const int& claimId) {
    debate::Claim claim = getClaimById(claimId);
    int parentId = claim.parent_id();
    if (parentId == -1){
        return claim; // root claim has no parent
    }
    return getClaimById(parentId);
}

void DebateWrapper::addClaimUnderParent(
    const int& parentId, 
    const std::string& claimText, 
    const std::string& description,
    const int& user_id,
    const int& debate_id) {

    debate::Claim parentClaimFromDB = getClaimById(parentId);

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
    int newId = databaseWrapper.statements.addStatement(
        -1, 
        claim.sentence(), 
        serializedData,
        user_id
    );
    claim.set_id(newId);
    std::vector<uint8_t> updatedSerializedData(claim.ByteSizeLong());
    claim.SerializeToArray(updatedSerializedData.data(), updatedSerializedData.size());
    databaseWrapper.statements.updateStatementProtobuf(
        claim.id(), 
        updatedSerializedData
    );
    databaseWrapper.statements.updateStatementRoot(
        claim.id(), 
        claim.id()
    );
}

void DebateWrapper::updateClaimInDB(const debate::Claim& claim) {
    std::vector<uint8_t> serializedData(claim.ByteSizeLong());
    claim.SerializeToArray(serializedData.data(), serializedData.size());
    databaseWrapper.statements.updateStatementProtobuf(
        claim.id(), 
        serializedData
    );
}

void DebateWrapper::deleteDebate(const int& debateId, const int& user_id) {
    // find the debate protobuf
    std::vector<uint8_t> debateData = databaseWrapper.debates.getDebateProtobuf(debateId);
    if (debateData.empty()) {
        Log::error("[DebateWrapper] Debate with ID " + std::to_string(debateId) + " not found.");
    }
    else {
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateData.data(), debateData.size());
        
        // find all debatemembers and delete
        databaseWrapper.debateMembers.removeAllMembersFromDebate(debateProto.root_claim_id());
        // delete all claims associated with the debate
        debate::Claim rootClaim = getClaimById(debateProto.root_claim_id());
        std::vector<int> claimsToDelete;
        claimsToDelete.push_back(debateProto.root_claim_id());

        size_t index = 0;
        while (index < claimsToDelete.size()) { // BFS-like traversal
            int currentClaimId = claimsToDelete[index];
            debate::Claim currentClaim = getClaimById(currentClaimId);
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
        databaseWrapper.debateMembers.removeMember(debateId, user_id);
    }
}

void DebateWrapper::deleteClaim(const int& claimId) {
    // find the claim parent to remove it's reference, dont actually delete the claim
    debate::Claim claim = getClaimById(claimId);
    int parentId = claim.parent_id();
    if (parentId != -1) {
        debate::Claim parentClaim = getClaimById(parentId);
        auto* proof = parentClaim.mutable_proof();
        auto& claimIds = *proof->mutable_claim_ids();
        claimIds.erase(
            std::remove(claimIds.begin(), claimIds.end(), claimId),
            claimIds.end()
        );
        updateClaimInDB(parentClaim);
    }

    // databaseWrapper.statements.deleteStatement(claimId);
}

void DebateWrapper::deleteAllDebates(const int& user_id) {
    std::vector<int> allDebates = databaseWrapper.debateMembers.getDebateIdsForUser(user_id);
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
    
    bool success = databaseWrapper.users.updateUserProtobuf(user_id, updatedData);
    
    if (success) {
        Log::debug("[DebateWrapper] Successfully moved user " + std::to_string(user_id) + " to claim id " + std::to_string(claimId) 
                    + " (state=DEBATING)");
    } else {
        Log::error("[DebateWrapper][ERR] Failed to update user protobuf for " + std::to_string(user_id));
    }
    
    return;
}

std::vector<int> DebateWrapper::getUserDebateIds(const int& user_id) {
    return databaseWrapper.debateMembers.getDebateIdsForUser(user_id);
}

std::vector<uint8_t> DebateWrapper::getDebateProtobuf(const int& debateId) {
    return databaseWrapper.debates.getDebateProtobuf(debateId);
}

std::vector<uint8_t> DebateWrapper::getUserProtobufBinary(const int& user_id) {
    return databaseWrapper.users.getUserProtobuf(user_id);
}

user::User DebateWrapper::getUserProtobuf(const int& user_id) {
    std::vector<uint8_t> userData = databaseWrapper.users.getUserProtobuf(user_id);
    user::User userProto;
    if (!userProto.ParseFromArray(userData.data(), static_cast<int>(userData.size()))) {
        Log::error("[DebateWrapper][ERR] Failed to parse user protobuf for " + std::to_string(user_id));
    }
    return userProto;
}

void DebateWrapper::updateUserProtobuf(const int& user_id, const user::User& userProto) {
    std::vector<uint8_t> protobufData(userProto.ByteSizeLong());
    userProto.SerializeToArray(protobufData.data(), protobufData.size());
    bool success = databaseWrapper.users.updateUserProtobuf(user_id, protobufData);
    if (success) {
        Log::debug("[DebateWrapper] Successfully updated user protobuf for " + std::to_string(user_id));
    } else {
        Log::error("[DebateWrapper][ERR] Failed to update user protobuf for " + std::to_string(user_id));
    }
}

void DebateWrapper::updateUserProtobufBinary(const int& user_id, const std::vector<uint8_t>& protobufData) {
    bool success = databaseWrapper.users.updateUserProtobuf(user_id, protobufData);
    if (success) {
        Log::debug("[DebateWrapper] Successfully updated user protobuf for " + std::to_string(user_id));
    } else {
        Log::error("[DebateWrapper][ERR] Failed to update user protobuf for " + std::to_string(user_id));
    }
}

void DebateWrapper::changeDescriptionOfClaim(
    const int& claimId,
    const std::string& newDescription) {
    debate::Claim claim = getClaimById(claimId);
    claim.set_description(newDescription);
    updateClaimInDB(claim);
}

void DebateWrapper::editClaimText(
    const int& claimId,
    const std::string& newText) {
    debate::Claim claim = getClaimById(claimId);
    claim.set_sentence(newText);
    updateClaimInDB(claim);
    // also update the actual topic in the statement database
    databaseWrapper.statements.updateStatementContent(claimId, newText);
}

int DebateWrapper::addLink(int fromClaimId, int toClaimId, const std::string& connection, int creator_id) {
    int linkId = databaseWrapper.links.addLink(fromClaimId, toClaimId, connection, creator_id);
    Log::debug("[DebateWrapper] Added link from claim " + std::to_string(fromClaimId) + " to claim " + std::to_string(toClaimId) + " by user " + std::to_string(creator_id));
    return linkId;
}

std::vector<int> DebateWrapper::findLinksUnder(const int& claimId) {
    // should be in the proof of the claim
    debate::Claim claim = getClaimById(claimId);
    std::vector<int> linkIds;
    for (const auto& linkId : claim.proof().link_ids()) {
        linkIds.push_back((linkId));
        Log::debug("[IMPROTATN] Found link ID: " + std::to_string(linkId) + " under Claim ID: " + std::to_string(claimId));
    }
    return linkIds;
}

debate::Link DebateWrapper::getLinkById(int linkId) {
    debate::Link linkProto;
    auto linkData = databaseWrapper.links.getLinkById(linkId);
    
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
    bool success = databaseWrapper.links.deleteLink(linkId);
    if (success) {
        Log::debug("[DebateWrapper] Successfully deleted link with ID " + std::to_string(linkId));
    } else {
        Log::error("[DebateWrapper][ERR] Failed to delete link with ID " + std::to_string(linkId));
    }
}

void DebateWrapper::addMemberToDebate(const int& debateId, const int& user_id) {
    // check if user already a member
    if (databaseWrapper.debateMembers.isMember(debateId, user_id)) {
        Log::info("[DebateWrapper] User " + std::to_string(user_id) + " is already a member of debate " + std::to_string(debateId));
        return;
    }
    // check if debate exists
    if (databaseWrapper.debates.debateExists(debateId) == false) {
        Log::error("[DebateWrapper][ERR] Debate with ID " + std::to_string(debateId) + " does not exist. Cannot add member.");
        return;
    }
    databaseWrapper.debateMembers.addMember(debateId, user_id);
}

int DebateWrapper::addChallenge(
    const int& creator_id,
    const int& challenged_claim_id,
    debate::Challenge challengeProtobuf
) {
    // serialize challengeProtobuf
    std::vector<uint8_t> challengeProtobufData(challengeProtobuf.ByteSizeLong());
    challengeProtobuf.SerializeToArray(challengeProtobufData.data(), challengeProtobufData.size());
    int challenge_id = databaseWrapper.challenges.addChallenge(
        challengeProtobufData,
        creator_id,
        challenged_claim_id
    );
    return challenge_id;
}

std::vector<int> DebateWrapper::getChallengesAgainstClaim(const int& claimId) {
    return databaseWrapper.challenges.getChallengesAgainstClaim(claimId);
}

debate::Challenge DebateWrapper::getChallengeProtobuf(int challengeId) {
    debate::Challenge challengeProto;
    std::vector<uint8_t> challengeData = databaseWrapper.challenges.getChallengeProtobuf(challengeId);
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
    bool success = databaseWrapper.challenges.deleteChallenge(challengeId);
    if (success) {
        Log::debug("[DebateWrapper] Successfully deleted challenge with ID " + std::to_string(challengeId));
    } else {
        Log::error("[DebateWrapper][ERR] Failed to delete challenge with ID " + std::to_string(challengeId));
    }
}

void DebateWrapper::updateDebateProtobuf(const int& debateId, const debate::Debate& debateProto) {
    std::vector<uint8_t> protobufData(debateProto.ByteSizeLong());
    debateProto.SerializeToArray(protobufData.data(), protobufData.size());
    databaseWrapper.debates.updateDebateProtobuf(debateId, debateProto.creator_id(), protobufData);
}

int DebateWrapper::findDebateId(const int& claimId) {
    debate::Claim claim = getClaimById(claimId);
    int debate_id = claim.debate_id();
    return debate_id;
}

user_engagement::DebateList DebateWrapper::FillUserDebateList(const int& user_id) {
    std::vector<int> debateIds = getUserDebateIds(user_id);

    user_engagement::DebateList debateListProto;

    for (const int& debateId : debateIds) {
        std::vector<uint8_t> debateBytes = getDebateProtobuf(debateId);
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateBytes.data(), debateBytes.size());
        user_engagement::DebateTopic* topicProto = debateListProto.add_topics();
        topicProto->set_id(debateProto.id());
        topicProto->set_topic(debateProto.topic());
        topicProto->set_creator_id(debateProto.creator_id());
        topicProto->set_is_challenge(debateProto.is_challenge());
        // find the claim debateProto is challenging
        getClaimById(debateProto.id());
        if (debateProto.is_challenge()){
            debate::Challenge challenge = getChallengeProtobuf(debateProto.parent_challenge_id());
            debate::Claim challengedClaim = getClaimById(challenge.challenged_parent_claim_id());
            topicProto->set_claim_its_challenging(challengedClaim.sentence());
        }
    }
    return debateListProto;
}

void DebateWrapper::SaveVersionOfClaim(const int& claim_id) {
    // find the claim proto
    debate::Claim claimProto = getClaimById(claim_id);
    // make a version to put in history, basically copy the current claim proto
    debate::ClaimState claimStateProto;
    // copy the proto
    claimStateProto.mutable_claim()->CopyFrom(claimProto);
    // get a timestamp
    int64_t timestamp = static_cast<int64_t>(time(nullptr));
    claimStateProto.mutable_timestamp()->set_seconds(timestamp);
    claimStateProto.mutable_timestamp()->set_nanos(0);
    claimProto.add_history()->CopyFrom(claimStateProto);
    // update the claim in DB
    updateClaimInDB(claimProto);
    // done
}

void DebateWrapper::RestorePreviousVersionOfClaim(const int& claim_id) {
    // find the most recent version from history
    debate::Claim claimProto = getClaimById(claim_id);
    if (claimProto.history_size() == 0) {
        Log::warn("[DebateWrapper] No previous versions found for claim ID " + std::to_string(claim_id));
        return;
    }
    debate::ClaimState lastVersion = claimProto.history(claimProto.history_size() - 1);
    
    // Save the current history before restoring
    auto savedHistory = claimProto.history();
    
    // restore the claim proto (this will overwrite history too)
    claimProto.CopyFrom(lastVersion.claim());
    
    // restore the history and remove the last entry
    *claimProto.mutable_history() = savedHistory;
    claimProto.mutable_history()->RemoveLast();
    
    // update the claim in DB
    updateClaimInDB(claimProto);
    // done
    // check child claims to see if correct
    for (const auto& childId : claimProto.proof().claim_ids()) {
        debate::Claim childClaim = getClaimById(childId);
        Log::test("[DebateWrapper] Claim Id: " + std::to_string(claim_id) + " Child Claim ID: " + std::to_string(childId) + " Parent ID after restoring: " + std::to_string(childClaim.parent_id()));
        if (childClaim.parent_id() != claim_id) {
            Log::warn("[DebateWrapper] Child claim ID " + std::to_string(childId) + " has incorrect parent ID " + std::to_string(childClaim.parent_id()) + " after restoring claim ID " + std::to_string(claim_id));
        }
    }
    // test 
    
} 