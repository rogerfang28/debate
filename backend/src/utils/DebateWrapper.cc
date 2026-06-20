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
    const auto links = databaseWrapper.links.getLinksFromClaim(parentId);
    for (const auto& linkRow : links) {
        const int linkId = std::get<0>(linkRow);
        const int linkTo = std::get<2>(linkRow);
        auto linkData = databaseWrapper.links.getLinkById(linkId);
        if (!linkData.has_value()) {
            continue;
        }

        const int linkType = std::get<5>(linkData.value());
        if (linkType != 1) {
            continue;
        }

        childrenIds.push_back(linkTo);
    }
    return childrenIds;
}

std::vector<std::pair<std::string,std::string>> DebateWrapper::findChildrenInfo(
    const int& parentId) {
    std::vector<std::pair<std::string,std::string>> childrenInfo;
    for (const auto& claim_id : findChildrenIds(parentId)) {
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

std::vector<int> DebateWrapper::findUsersInDebate(const int& debate_id) {
    return databaseWrapper.debateMembers.getUserIdsForDebate(debate_id);
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
    rootClaim.set_creator_id(creator_id);
    addClaimToDB(rootClaim, creator_id, newId);
    debate.set_root_claim_id(rootClaim.id());
    std::vector<uint8_t> updatedSerializedDebate(debate.ByteSizeLong());
    debate.SerializeToArray(updatedSerializedDebate.data(), updatedSerializedDebate.size());
    databaseWrapper.debates.updateDebateProtobuf(debate.id(), creator_id, updatedSerializedDebate);
    databaseWrapper.debateMembers.addMember(newId, creator_id);
}

debate::Claim DebateWrapper::findClaimParent(const int& claimId) {
    debate::Claim claim = getClaimById(claimId);
    // Resolve parent by finding an incoming parent-child link.
    const auto links = databaseWrapper.links.getLinksForClaim(claimId);
    for (const auto& linkRow : links) {
        const int linkId = std::get<0>(linkRow);
        const int linkFrom = std::get<1>(linkRow);
        const int linkTo = std::get<2>(linkRow);

        if (linkTo != claimId) {
            continue;
        }

        auto linkData = databaseWrapper.links.getLinkById(linkId);
        if (!linkData.has_value()) {
            continue;
        }

        const int linkType = std::get<5>(linkData.value());
        if (linkType != 1) {
            // In the current schema, 1 is the parent-child link type.
            continue;
        }

        debate::Claim parentClaim = getClaimById(linkFrom);
        if (parentClaim.id() == 0) {
            Log::warn("[DebateWrapper] Parent claim " + std::to_string(linkFrom) + " not found while resolving parent of claim " + std::to_string(claimId));
            return claim;
        }

        return parentClaim;
    }

    // Root claim or stale data with no parent-child link.
    return claim;
}

bool DebateWrapper::isRoot(const int& claimId) {
    return findClaimParent(claimId).id() == claimId;
}

int DebateWrapper::createClaim(
    const std::string& claimText,
    const std::string& description,
    const int& user_id,
    const int& debate_id) {
    debate::Claim claim;
    claim.set_sentence(claimText);
    claim.set_description(description);
    claim.set_creator_id(user_id);
    claim.set_debate_id(debate_id);
    claim.set_status(debate::ClaimStatus::UNDETERMINED);
    addClaimToDB(claim, user_id, debate_id);
    return claim.id();
}

int DebateWrapper::addClaimUnderParent(
    const int& parentId, 
    const std::string& claimText, 
    const std::string& description,
    const int& user_id,
    const int& debate_id) {

    // debate::Claim parentClaimFromDB = getClaimById(parentId);

    // Add new claim
    debate::Claim childClaim;
    childClaim.set_sentence(claimText);
    childClaim.set_description(description);
    childClaim.set_creator_id(user_id);
    childClaim.set_debate_id(debate_id);
    childClaim.set_status(debate::ClaimStatus::UNDETERMINED);
    addClaimToDB(childClaim, user_id, debate_id);
    addLink(parentId, childClaim.id(), "parent to child connection", user_id, debate_id, debate::LinkType::PARENT_CHILD);
    Log::debug("Added link between claim " + std::to_string(parentId) + " and new claim " + std::to_string(childClaim.id()) + " with description: " + "parent child connection");
    return childClaim.id();
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
        user_id,
        debate_id
    );
    claim.set_id(newId);
    std::vector<uint8_t> updatedSerializedData(claim.ByteSizeLong());
    claim.SerializeToArray(updatedSerializedData.data(), updatedSerializedData.size());
    databaseWrapper.statements.updateStatementProtobuf(
        claim.id(), 
        updatedSerializedData
    );
    databaseWrapper.statements.updateStatementDebateId(claim.id(), debate_id);
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
            for (const auto& childId : findChildrenIds(currentClaimId)) {
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
    debate::Claim claim = getClaimById(claimId);
    const auto links = databaseWrapper.links.getLinksForClaim(claimId);

    for (const auto& linkRow : links) {
        const int linkId = std::get<0>(linkRow);
        const int linkFrom = std::get<1>(linkRow);
        const int linkTo = std::get<2>(linkRow);

        auto linkData = databaseWrapper.links.getLinkById(linkId);
        if (!linkData.has_value()) {
            continue;
        }

        const int linkType = std::get<5>(linkData.value());
        if (linkType != 1) {
            continue;
        }

        if (linkTo == claimId) {
            debate::Claim parentClaim = getClaimById(linkFrom);
            updateClaimInDB(parentClaim);
        }

        if (linkFrom == claimId) {
            debate::Claim childClaim = getClaimById(linkTo);
            updateClaimInDB(childClaim);
        }

        deleteLinkById(linkId);
    }

    // databaseWrapper.statements.deleteStatement(claimId); 
    // this line was commented out because we dont actually want to delete from the database, just make it so its no longer accessible
    // this also means we need to remove the new link of parent child for this as well
    
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

int DebateWrapper::addLink(int fromClaimId, int toClaimId, const std::string& connection, int creator_id, int debate_id, debate::LinkType link_type) {
    int linkId = databaseWrapper.links.addLink(fromClaimId, toClaimId, connection, creator_id, debate_id, static_cast<int>(link_type));
    if (linkId == -1) {
        Log::error("[DebateWrapper][ERR] Failed to add link from claim " + std::to_string(fromClaimId)
            + " to claim " + std::to_string(toClaimId));
        return -1;
    }

    auto addTopLevelLinkIdIfMissing = [linkId](debate::Claim& claim) {
        for (const auto& existingLinkId : claim.link_ids()) {
            if (existingLinkId == linkId) {
                return;
            }
        }
        claim.add_link_ids(linkId);
    };

    debate::Claim fromClaim = getClaimById(fromClaimId);
    addTopLevelLinkIdIfMissing(fromClaim);
    updateClaimInDB(fromClaim);

    if (toClaimId != fromClaimId) {
        debate::Claim toClaim = getClaimById(toClaimId);
        addTopLevelLinkIdIfMissing(toClaim);
        updateClaimInDB(toClaim);
    }

    Log::debug("[DebateWrapper] Added link from claim " + std::to_string(fromClaimId) + " to claim " + std::to_string(toClaimId) + " by user " + std::to_string(creator_id));
    return linkId;
}

std::vector<std::vector<uint8_t>> DebateWrapper::getStatementsForDebateAndCreators(const int& debate_id, const std::vector<int>& creator_ids) {
    return databaseWrapper.statements.getStatementsForDebateAndCreators(debate_id, creator_ids);
}

std::vector<std::vector<uint8_t>> DebateWrapper::getStatementsForDebate(const int& debate_id) {
    return databaseWrapper.statements.getStatementsForDebate(debate_id);
}

std::vector<std::tuple<int, int, int, std::string, int, int>> DebateWrapper::getLinksForDebateAndCreators(const int& debate_id, const std::vector<int>& creator_ids) {
    return databaseWrapper.links.getLinksForDebateAndCreators(debate_id, creator_ids);
}

std::vector<std::tuple<int, int, int, std::string, int, int>> DebateWrapper::getLinksForDebate(const int& debate_id) {
    return databaseWrapper.links.getLinksForDebate(debate_id);
}

std::vector<int> DebateWrapper::findLinksUnder(const int& claimId) {
    debate::Claim claim = getClaimById(claimId);
    std::vector<int> linkIds;
    for (const auto& linkId : claim.link_ids()) {
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
        linkProto.set_creator_id(std::get<4>(link));
        linkProto.set_link_type(static_cast<debate::LinkType>(std::get<5>(link)));
        
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
        Log::debug("[DebateWrapper] User " + std::to_string(user_id) + " is already a member of debate " + std::to_string(debateId));
        return;
    }
    // check if debate exists
    if (databaseWrapper.debates.debateExists(debateId) == false) {
        Log::error("[DebateWrapper][ERR] Debate with ID " + std::to_string(debateId) + " does not exist. Cannot add member.");
        return;
    }
    databaseWrapper.debateMembers.addMember(debateId, user_id);
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
        topicProto->set_is_challenge(false);
        topicProto->set_claim_its_challenging("");
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
    for (const auto& childId : findChildrenIds(claim_id)) {
        debate::Claim childClaim = getClaimById(childId);
        debate::Claim resolvedParent = findClaimParent(childId);
        Log::debug("[DebateWrapper] Claim Id: " + std::to_string(claim_id) + " Child Claim ID: " + std::to_string(childId) + " resolved parent after restoring: " + std::to_string(resolvedParent.id()));
        if (resolvedParent.id() != claim_id) {
            Log::warn("[DebateWrapper] Child claim ID " + std::to_string(childId) + " has incorrect resolved parent " + std::to_string(resolvedParent.id()) + " after restoring claim ID " + std::to_string(claim_id));
        }
    }
    // test 
    
}

void DebateWrapper::UpdateStatusOfAllClaimsInDebate(const int& debate_id) {
    // New model: UNDETERMINED (default), TRUE (defended), FALSE (disproven).
    // No CHALLENGED status — challenge resolution is determined by
    // the challenge claim's own status.
    // For each claim with incoming CHALLENGE links:
    //   - If any challenge claim is FALSE → target is FALSE (disproven)
    //   - If any challenge claim is TRUE → target is TRUE (defended)
    //   - If all challenge claims are UNDETERMINED → target stays UNDETERMINED
    // Propagate FALSE up to ancestors.
    debate::Debate debateProto;
    std::vector<uint8_t> debateData = databaseWrapper.debates.getDebateProtobuf(debate_id);
    if (!debateData.empty()) {
        debateProto.ParseFromArray(debateData.data(), debateData.size());
    } else {
        Log::error("[DebateWrapper][ERR] Debate with ID " + std::to_string(debate_id) + " not found.");
        return;
    }

    // First pass: determine each claim's status based on incoming challenges.
    std::vector<int> stack;
    std::set<int> visited;
    stack.push_back(debateProto.root_claim_id());

    while (!stack.empty()) {
        int currentClaimId = stack.back();
        stack.pop_back();

        if (visited.find(currentClaimId) != visited.end()) {
            continue;
        }

        visited.insert(currentClaimId);
        debate::Claim currentClaim = getClaimById(currentClaimId);

        // Check incoming CHALLENGE links to determine status.
        bool hasIncomingChallenge = false;
        bool anyChallengeFalse = false;
        bool anyChallengeTrue = false;
        const auto links = databaseWrapper.links.getLinksForClaim(currentClaimId);
        for (const auto& linkRow : links) {
            const int linkTo = std::get<2>(linkRow);
            if (linkTo != currentClaimId) {
                continue;
            }

            const int linkId = std::get<0>(linkRow);
            auto linkData = databaseWrapper.links.getLinkById(linkId);
            if (!linkData.has_value()) {
                continue;
            }

            const int linkType = std::get<5>(linkData.value());
            if (linkType == static_cast<int>(debate::LinkType::CHALLENGE)) {
                hasIncomingChallenge = true;
                // The challenge claim is the connect_from side.
                const int challengeClaimId = std::get<1>(linkRow);
                debate::Claim challengeClaim = getClaimById(challengeClaimId);
                if (challengeClaim.status() == debate::ClaimStatus::DISPROVEN) {
                    anyChallengeFalse = true;
                } else if (challengeClaim.status() == debate::ClaimStatus::UPHELD) {
                    anyChallengeTrue = true;
                }
            }
        }

        if (hasIncomingChallenge) {
            if (anyChallengeFalse) {
                currentClaim.set_status(debate::ClaimStatus::DISPROVEN);
            } else if (anyChallengeTrue) {
                currentClaim.set_status(debate::ClaimStatus::UPHELD);
            }
            // If all challenges are UNDETERMINED, leave as UNDETERMINED.
        }
        // If no incoming challenges, leave as-is (UNDETERMINED default).
        updateClaimInDB(currentClaim);

        for (const auto& childId : findChildrenIds(currentClaimId)) {
            if (visited.find(childId) == visited.end()) {
                stack.push_back(childId);
            }
        }
    }

    // Second pass: propagate FALSE up to ancestors.
    // If any child is FALSE, parent becomes FALSE too.
    std::vector<int> toProcess;
    toProcess.push_back(debateProto.root_claim_id());

    for (size_t i = 0; i < toProcess.size(); ++i) {
        int currentClaimId = toProcess[i];
        debate::Claim currentClaim = getClaimById(currentClaimId);
        std::vector<int> childrenIds = findChildrenIds(currentClaimId);

        for (const auto& childId : childrenIds) {
            toProcess.push_back(childId);
        }

        bool hasChildFalse = false;
        for (const auto& childId : childrenIds) {
            debate::Claim childClaim = getClaimById(childId);
            if (childClaim.status() == debate::ClaimStatus::DISPROVEN) {
                hasChildFalse = true;
                break;
            }
        }

        if (hasChildFalse && currentClaim.status() != debate::ClaimStatus::DISPROVEN) {
            currentClaim.set_status(debate::ClaimStatus::DISPROVEN);
            updateClaimInDB(currentClaim);
        }
    }
}