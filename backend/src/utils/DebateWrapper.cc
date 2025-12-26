#include "DebateWrapper.h"
#include <iostream>
#include "Log.h"
#include "../../../src/gen/cpp/debate.pb.h"
#include "../../../src/gen/cpp/user.pb.h"

DebateWrapper::DebateWrapper(DebateDatabase& debateDatabase, StatementDatabase& statementDatabase, UserDatabase& userDatabase, DebateMembersDatabase& debateMembersDatabase)
    : debateDb(debateDatabase),
      statementDb(statementDatabase),
      userDb(userDatabase),
      debateMembersDb(debateMembersDatabase) {}

std::vector<std::string> DebateWrapper::findChildrenIds(
    const std::string& parentId) {
    std::vector<std::string> childrenIds;
    debate::Claim parentClaim = findClaim(parentId);
    for (const auto& childId : parentClaim.proof().claim_ids()) {
        childrenIds.push_back(childId);
    }
    return childrenIds;
}

std::vector<std::pair<std::string,std::string>> DebateWrapper::findChildrenInfo(
    const std::string& parentId) {
    std::vector<std::pair<std::string,std::string>> childrenInfo;
    debate::Claim parentClaim = findClaim(parentId);

    for (const auto& claim_id: parentClaim.proof().claim_ids()) {
        debate::Claim claim = findClaim(claim_id);
        childrenInfo.emplace_back(claim.id(), claim.sentence());
    }
    return childrenInfo;
}

std::string DebateWrapper::findClaimSentence(
    const std::string& claimId) {
    debate::Claim claim = findClaim(claimId);
    return claim.sentence();
}

debate::Claim DebateWrapper::findClaim(const std::string& claimId) {
    std::vector<uint8_t> serializedData = statementDb.getStatementProtobuf(std::stoi(claimId));
    if (!serializedData.empty()) {
        debate::Claim claim;
        claim.ParseFromArray(serializedData.data(), serializedData.size());
        return claim;
    }
    // return an empty Claim if not found
    Log::warn("[DebateWrapper] Claim with ID " + claimId + " not found in database.");
    return debate::Claim();
}

void DebateWrapper::initNewDebate(const std::string& topic, const std::string& owner) {
    debate::Claim rootClaim;
    rootClaim.set_sentence(topic);
    addClaimToDB(rootClaim);
    debate::Debate debate;
    debate.set_root_claim_id(rootClaim.id());
    debate.set_topic(topic);
    debate.add_debaters(owner);
    debate.set_owner(owner);
    std::vector<uint8_t> serialized_debate(debate.ByteSizeLong());
    debate.SerializeToArray(serialized_debate.data(), serialized_debate.size());
    int newId = debateDb.addDebate(owner, topic, serialized_debate);
    if (newId == -1) {
        Log::error("[DebateWrapper] Failed to create new debate for topic: " + topic);
        return;
    }
    debate.set_id(std::to_string(newId));
    std::vector<uint8_t> updatedSerializedDebate(debate.ByteSizeLong());
    debate.SerializeToArray(updatedSerializedDebate.data(), updatedSerializedDebate.size());
    debateDb.updateDebateProtobuf(std::stoi(debate.id()), owner, updatedSerializedDebate);
    debateMembersDb.addMember(debate.root_claim_id(), owner);
}



debate::Claim DebateWrapper::findClaimParent(const std::string& claimId) {
    debate::Claim claim = findClaim(claimId);
    std::string parentId = claim.parent_id();
    return findClaim(parentId);
}

void DebateWrapper::addClaimUnderParent(
    const std::string& parentId, 
    const std::string& claimText, 
    const std::string& connectionToParent) {

    debate::Claim parentClaimFromDB = findClaim(parentId);

    // Add new claim
    debate::Claim childClaim;
    childClaim.set_sentence(claimText);
    childClaim.set_parent_id(parentId);
    addClaimToDB(childClaim);
    parentClaimFromDB.mutable_proof()->add_claim_ids(childClaim.id());
    updateClaimInDB(parentClaimFromDB);
}

void DebateWrapper::addClaimToDB(debate::Claim& claim) {
    std::vector<uint8_t> serializedData(claim.ByteSizeLong());
    claim.SerializeToArray(serializedData.data(), serializedData.size());
    int newId = statementDb.addStatement(
        -1, 
        claim.sentence(), 
        serializedData
    );
    claim.set_id(std::to_string(newId));
    std::vector<uint8_t> updatedSerializedData(claim.ByteSizeLong());
    claim.SerializeToArray(updatedSerializedData.data(), updatedSerializedData.size());
    statementDb.updateStatementProtobuf(
        std::stoi(claim.id()), 
        updatedSerializedData
    );
    statementDb.updateStatementRoot(
        std::stoi(claim.id()), 
        std::stoi(claim.id())
    );
}

void DebateWrapper::updateClaimInDB(const debate::Claim& claim) {
    std::vector<uint8_t> serializedData(claim.ByteSizeLong());
    claim.SerializeToArray(serializedData.data(), serializedData.size());
    statementDb.updateStatementProtobuf(
        std::stoi(claim.id()), 
        serializedData
    );
}

void DebateWrapper::deleteDebate(const std::string& debateId, const std::string& user) {
    // find the debate protobuf
    std::vector<uint8_t> debateData = debateDb.getDebateProtobuf(std::stoi(debateId));
    if (debateData.empty()) {
        Log::error("[DebateWrapper] Debate with ID " + debateId + " not found.");
    }
    else {
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateData.data(), debateData.size());
        
        // find all debatemembers and delete
        debateMembersDb.removeAllMembersFromDebate(debateProto.root_claim_id());
        // delete all claims associated with the debate
        debate::Claim rootClaim = findClaim(debateProto.root_claim_id());
        std::vector<std::string> claimsToDelete;
        claimsToDelete.push_back(debateProto.root_claim_id());

        size_t index = 0;
        while (index < claimsToDelete.size()) { // BFS-like traversal
            std::string currentClaimId = claimsToDelete[index];
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
        debateDb.removeDebate(std::stoi(debateId), user);
    }
}

void DebateWrapper::deleteClaim(const std::string& claimId) {
    statementDb.deleteStatement(std::stoi(claimId));
}

void DebateWrapper::deleteAllDebates(const std::string& user) {
    std::vector<std::string> allDebates = debateMembersDb.getDebateIdsForUser(user);
    for (const auto& debateId : allDebates) {
        deleteDebate(debateId, user);
    }
}

void DebateWrapper::moveUserToClaim(const std::string& username, const std::string& claimId) {
    std::vector<uint8_t> userData = userDb.getUserProtobufByUsername(username);

    user::User userProto;
    if (!userProto.ParseFromArray(userData.data(), static_cast<int>(userData.size()))) {
        Log::error("[DebateWrapper][ERR] Failed to parse user protobuf for " + username);
        return;
    }

    userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_DEBATING);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_root_claim()->set_id(claimId);
    user_engagement::ClaimInfo currentClaim;
    currentClaim.set_id(claimId);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->CopyFrom(currentClaim);
    
    std::vector<uint8_t> updatedData(userProto.ByteSizeLong());
    userProto.SerializeToArray(updatedData.data(), updatedData.size());
    
    bool success = userDb.updateUserProtobuf(username, updatedData);
    
    if (success) {
        Log::debug("[DebateWrapper] Successfully moved user " + username + " to claim id " + claimId 
                    + " (state=DEBATING)");
    } else {
        Log::error("[DebateWrapper][ERR] Failed to update user protobuf for " + username);
    }
    
    return;
}

std::vector<std::string> DebateWrapper::getUserDebateIds(const std::string& user) {
    return debateMembersDb.getDebateIdsForUser(user);
}

std::vector<uint8_t> DebateWrapper::getDebateProtobuf(const std::string& debateId) {
    return debateDb.getDebateProtobuf(std::stoi(debateId));
}

std::vector<uint8_t> DebateWrapper::getUserProtobuf(const std::string& user) {
    return userDb.getUserProtobuf(user);
}

std::vector<uint8_t> DebateWrapper::getUserProtobufByUsername(const std::string& username) {
    return userDb.getUserProtobufByUsername(username);
}

void DebateWrapper::updateUserProtobuf(const std::string& user, const std::vector<uint8_t>& protobufData) {
    bool success = userDb.updateUserProtobuf(user, protobufData);
    if (success) {
        Log::debug("[DebateWrapper] Successfully updated user protobuf for " + user);
    } else {
        Log::error("[DebateWrapper][ERR] Failed to update user protobuf for " + user);
    }
}