#include "DebateWrapper.h"
#include <iostream>
#include <map>
#include <algorithm>
#include "Log.h"
#include "RelationBuilder.h"
#include "debate.pb.h"
#include "user.pb.h"

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

void DebateWrapper::initNewDebate(const std::string& topic, const int& creator_id, int voice_id,
                                  const debate::Source* source) {
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
    rootClaim.set_voice_id(voice_id);
    if (source != nullptr && (!source->url().empty() || !source->excerpt().empty())) {
        *rootClaim.add_sources() = *source;
    }

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
    const int& debate_id,
    int voice_id,
    const debate::Source* source) {
    debate::Claim claim;
    claim.set_sentence(claimText);
    claim.set_description(description);
    claim.set_creator_id(user_id);
    claim.set_debate_id(debate_id);
    claim.set_voice_id(voice_id);
    if (source != nullptr && (!source->url().empty() || !source->excerpt().empty())) {
        *claim.add_sources() = *source;
    }

    // Whoever the claim belongs to sees it TRUE; addClaimToDB seeds that.
    addClaimToDB(claim, user_id, debate_id);
    return claim.id();
}

int DebateWrapper::addClaimUnderParent(
    const int& parentId,
    const std::string& claimText,
    const std::string& description,
    const int& user_id,
    const int& debate_id,
    const std::string& basis,
    int relation_id,
    int* out_relation_id,
    int voice_id,
    const debate::Source* source) {

    // debate::Claim parentClaimFromDB = getClaimById(parentId);

    // Add new claim
    debate::Claim childClaim;
    childClaim.set_sentence(claimText);
    childClaim.set_description(description);
    childClaim.set_creator_id(user_id);
    childClaim.set_debate_id(debate_id);
    childClaim.set_voice_id(voice_id);
    if (source != nullptr && (!source->url().empty() || !source->excerpt().empty())) {
        *childClaim.add_sources() = *source;
    }

    // Whoever the claim belongs to sees it TRUE; addClaimToDB seeds that.
    addClaimToDB(childClaim, user_id, debate_id);

    // The link's `connection` carries the basis. It previously held a fixed
    // string that told a reader nothing, so nothing is lost by using it, and
    // this is where the basis belongs until relations get their own row.
    const std::string connection = basis.empty() ? "parent to child connection" : basis;
    const int linkId = addLink(parentId, childClaim.id(), connection, user_id, debate_id,
                               debate::LinkType::PARENT_CHILD, relation_id);

    // Report the relation this row belongs to, so a caller adding several
    // claims to ONE relation can pass it back in for the rest. A row that is
    // its own relation has RELATION_ID == its own id.
    if (out_relation_id != nullptr) {
        *out_relation_id = relation_id > 0 ? relation_id : linkId;
    }

    Log::debug("Added link between claim " + std::to_string(parentId) + " and new claim " +
               std::to_string(childClaim.id()) + " with basis: " + connection +
               " (relation " + std::to_string(relation_id > 0 ? relation_id : linkId) + ")");
    return childClaim.id();
}

void DebateWrapper::addClaimToDB(debate::Claim& claim, const int& user_id, const int& debate_id) {
    claim.set_creator_id(user_id);
    claim.set_debate_id(debate_id);
    // No status is written. Who holds the claim true is the ASSERT move the
    // logger appends for this event; the page replays the log.
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
        // Delete both PARENT_CHILD and CHALLENGE links touching this claim --
        // otherwise deleting a challenge-root claim leaves its (now dangling)
        // challenge link behind instead of cleaning it up like a normal edge.
        if (linkType != debate::LinkType::PARENT_CHILD && linkType != debate::LinkType::CHALLENGE) {
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

std::vector<int> DebateWrapper::getAllDebateIds() {
    return databaseWrapper.debates.allDebateIds();
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

int DebateWrapper::addLink(int fromClaimId, int toClaimId, const std::string& connection, int creator_id, int debate_id, debate::LinkType link_type, int relation_id, int to_link_id) {
    int linkId = databaseWrapper.links.addLink(fromClaimId, toClaimId, connection, creator_id, debate_id, static_cast<int>(link_type), relation_id, to_link_id);
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

std::vector<std::tuple<int, int, int, std::string, int, int, int>> DebateWrapper::getLinksForDebateAndCreators(const int& debate_id, const std::vector<int>& creator_ids) {
    return databaseWrapper.links.getLinksForDebateAndCreators(debate_id, creator_ids);
}

std::vector<std::tuple<int, int, int, std::string, int, int, int>> DebateWrapper::getLinksForDebate(const int& debate_id) {
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

debate::Relationship DebateWrapper::getLinkById(int linkId) {
    debate::Relationship relationshipProto;
    auto linkData = databaseWrapper.links.getLinkById(linkId);

    if (linkData.has_value()) {
        const auto& link = linkData.value();
        relationshipProto = RelationBuilder::fromLinkRow(
            std::get<0>(link), std::get<1>(link), std::get<2>(link),
            std::get<3>(link), std::get<4>(link), std::get<5>(link),
            /*debateId=*/0, std::get<6>(link),
            databaseWrapper.links.getLinkTargetRelation(linkId));


        Log::debug("[DebateWrapper] Retrieved link ID: " + std::to_string(std::get<0>(link))
            + " from Claim ID: " + std::to_string(std::get<1>(link))
            + " to Claim ID: " + std::to_string(std::get<2>(link))
            + " with connection: " + std::get<3>(link)
            + " created by: " + std::to_string(std::get<4>(link)));
    } else {
        Log::warn("[DebateWrapper] Link with ID " + std::to_string(linkId) + " not found.");
    }

    return relationshipProto;
}

std::map<int, int> DebateWrapper::getRelationTargets(const int& debate_id) {
    return databaseWrapper.links.getRelationTargets(debate_id);
}

debate::Relationship DebateWrapper::findOutgoingChallengeLink(int fromClaimId) {
    debate::Claim claim = getClaimById(fromClaimId);
    for (int i = 0; i < claim.link_ids_size(); ++i) {
        debate::Relationship rel = getLinkById(claim.link_ids(i));
        const debate::Relationship::Link& link = rel.link();
        if (link.link_type() == debate::LinkType::CHALLENGE && link.connect_from() == fromClaimId) {
            return rel;
        }
    }
    return debate::Relationship();
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

std::vector<debate::Move> DebateWrapper::getMovesForDebate(const int& debate_id) {
    return databaseWrapper.moves.getMovesForDebate(debate_id);
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

void DebateWrapper::appendChatMessage(const int& user_id, const int& debate_id,
                                      const std::string& role, const std::string& text) {
    databaseWrapper.chats.append(user_id, debate_id, role, text);
}

std::vector<ChatDatabase::Message> DebateWrapper::chatTranscript(const int& user_id,
                                                                 const int& debate_id) {
    return databaseWrapper.chats.list(user_id, debate_id);
}

void DebateWrapper::clearChat(const int& user_id, const int& debate_id) {
    databaseWrapper.chats.clear(user_id, debate_id);
}

bool DebateWrapper::briefFor(const int& debate_id, std::string* fingerprint, std::string* text) {
    return databaseWrapper.briefs.load(debate_id, fingerprint, text);
}

void DebateWrapper::saveBrief(const int& debate_id, const std::string& fingerprint,
                              const std::string& text) {
    databaseWrapper.briefs.save(debate_id, fingerprint, text);
}
