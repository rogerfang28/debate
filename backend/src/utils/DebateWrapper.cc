#include "DebateWrapper.h"
#include <iostream>
#include "Log.h"

DebateWrapper::DebateWrapper() {}

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
    std::vector<uint8_t> serializedData = statementDBHandler.getStatementProtobuf(claimId);
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
    
    debateDBHandler.addDebateWithProtobuf(owner, topic, serialized_debate);

    debateMembersDBHandler.addMember(debate.root_claim_id(), owner);
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
    int newId = statementDBHandler.addStatement(
        claim.id(), 
        claim.sentence(), 
        serializedData
    );
    claim.set_id(std::to_string(newId));
    std::vector<uint8_t> updatedSerializedData(claim.ByteSizeLong());
    claim.SerializeToArray(updatedSerializedData.data(), updatedSerializedData.size());
    statementDBHandler.updateStatementProtobuf(
        claim.id(), 
        updatedSerializedData
    );
}

void DebateWrapper::updateClaimInDB(const debate::Claim& claim) {
    std::vector<uint8_t> serializedData(claim.ByteSizeLong());
    claim.SerializeToArray(serializedData.data(), serializedData.size());
    statementDBHandler.updateStatementProtobuf(
        claim.id(), 
        serializedData
    );
}

void DebateWrapper::deleteDebate(const std::string& debateId, const std::string& user) {
    // find the debate protobuf
    std::vector<uint8_t> debateData = debateDBHandler.getDebateProtobuf(debateId);
    if (debateData.empty()) {
        Log::error("[DebateWrapper] Debate with ID " + debateId + " not found.");
    }
    else {
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateData.data(), debateData.size());
        
        // find all debatemembers and delete
        debateMembersDBHandler.removeAllMembersFromDebate(debateProto.root_claim_id());
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
        debateDBHandler.removeDebate(debateId, user);
    }
}

void DebateWrapper::deleteClaim(const std::string& claimId) {
    statementDBHandler.removeStatement(claimId);
}

void DebateWrapper::deleteAllDebates(const std::string& user) {
    std::vector<std::map<std::string, std::string>> allDebates = debateDBHandler.getDebates(user);
    for (const auto& debateRow : allDebates) {
        std::string debateId = debateRow.at("ID");
        deleteDebate(debateId, user);
    }
}