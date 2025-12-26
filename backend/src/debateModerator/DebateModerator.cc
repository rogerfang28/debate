// should take in debate event protobuf and handle it and give back information
#include "DebateModerator.h"
// #include "../database/handlers/UserDatabaseHandler.h"
#include "../../../src/gen/cpp/debate.pb.h"
// #include "../../../src/gen/cpp/debate_list.pb.h"
#include "../../../src/gen/cpp/user.pb.h"
#include "../utils/pathUtils.h"
#include <iostream>
#include <vector>
#include "event-handlers/AddDebate/AddDebateHandler.h"
#include "event-handlers/ClearDebates/ClearDebatesHandler.h"
#include "event-handlers/DeleteDebate/DeleteDebateHandler.h"
#include "event-handlers/EnterDebate/EnterDebateHandler.h"
#include "event-handlers/GoToClaim/GoToClaim.h"
#include "event-handlers/GoHome/GoHomeHandler.h"
#include "event-handlers/GoToParentClaim/GoToParentClaim.h"
#include "event-handlers/AddClaimUnderClaim/AddClaimUnderClaim.h"
#include "buildResponse/homePageResponse/HomePageResponseGenerator.h"
#include "buildResponse/debatePageResponse/DebatePageResponseGenerator.h"
#include "../utils/Log.h"

DebateModerator::DebateModerator()
    : globalDb(utils::getDatabasePath()),
      userDb(globalDb),
      debateDb(globalDb),
      statementDb(globalDb),
      debateMembersDb(globalDb),
      debateWrapper(debateDb, statementDb, userDb, debateMembersDb)
{
    Log::debug("[DebateModerator] Initialized.");
}

DebateModerator::~DebateModerator() {
    Log::debug("[DebateModerator] Destroyed.");
}

// * main function to handle debate event requests
moderator_to_vr::ModeratorToVRMessage DebateModerator::handleRequest(const std::string& user, debate_event::DebateEvent& event){
    // Process the debate event and update the database accordingly
    Log::debug("[DebateModerator] Handling request for user: " + user);
    // Example: handle adding a debate topic
    handleDebateEvent(user, event);
    moderator_to_vr::ModeratorToVRMessage res = buildResponseMessage(user);
    return res;
}

void DebateModerator::handleDebateEvent(const std::string& user, debate_event::DebateEvent& event) {
    // Determine the type of event and call the appropriate handler
    switch (event.type()) {
        case debate_event::NONE:
            Log::debug("[DebateModerator] Event Type: NONE");
            break;
        case debate_event::CREATE_DEBATE:
            Log::debug("[DebateModerator] Event Type: CREATE_DEBATE");
            AddDebateHandler::AddDebate(event.create_debate().debate_topic(), user, debateWrapper);
            break;
        case debate_event::CLEAR_DEBATES:
            Log::debug("[DebateModerator] Event Type: CLEAR_DEBATES");
            ClearDebatesHandler::ClearDebates(user, debateWrapper);
            break;
        case debate_event::DELETE_DEBATE:
            Log::debug("[DebateModerator] Event Type: DELETE_DEBATE");
            DeleteDebateHandler::DeleteDebate(event.delete_debate().debate_id(), user, debateWrapper);
            break;
        case debate_event::ENTER_DEBATE:
            Log::debug("[DebateModerator] Event Type: ENTER_DEBATE");
            EnterDebateHandler::EnterDebate(event.enter_debate().debate_id(), user, debateWrapper);
            break;
        case debate_event::GO_HOME:
            Log::debug("[DebateModerator] Event Type: GO_HOME");
            GoHomeHandler::GoHome(user);
            break;
        case debate_event::GO_TO_PARENT:
            Log::debug("[DebateModerator] Event Type: GO_TO_PARENT");
            GoToParentClaimHandler::GoToParentClaim(user, debateWrapper);
            break;
        case debate_event::GO_TO_CLAIM:
            Log::debug("[DebateModerator] Event Type: GO_TO_CLAIM");
            GoToClaimHandler::GoToClaim(event.go_to_claim().claim_id(), user);
            break;
        case debate_event::OPEN_ADD_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: OPEN_ADD_CHILD_CLAIM");
            // implement later
            break;
        case debate_event::ADD_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: ADD_CHILD_CLAIM");
            AddClaimUnderClaimHandler::AddClaimUnderClaim(
                event.add_child_claim().claim(),
                event.add_child_claim().connection_to_parent(),
                user,
                debateWrapper
            );
            break;
        case debate_event::REPORT_CLAIM:
            Log::debug("[DebateModerator] Event Type: REPORT_CLAIM");
            // implement later
            break;
        case debate_event::DELETE_CURRENT_STATEMENT:
            Log::debug("[DebateModerator] Event Type: DELETE_CURRENT_STATEMENT");
            // implement later
            break;
        default:
            Log::debug("[DebateModerator] Event Type: UNKNOWN");
            break;
    }
}

moderator_to_vr::ModeratorToVRMessage DebateModerator::buildResponseMessage(const std::string& user) {
    moderator_to_vr::ModeratorToVRMessage responseMessage;
    // Build the response message based on the current state
    // For example, populate user engagement and debate information
    // so i have to first access the database to get the information about the user engagement
    // UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    user::User userProto;
    
    // user doesn't exist yet, make a default user
    if (!userDb.userExists(user)) {
        userProto.set_username(user);
        userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_NONE);
        userProto.mutable_engagement()->mutable_none_info();

        std::vector<uint8_t> userData(userProto.ByteSizeLong());
        userProto.SerializeToArray(userData.data(), userData.size());
        userDb.createUser(user, userData);
    }

    // now we get info from the database
    std::vector<uint8_t> userData = userDb.getUserProtobuf(user);
    userProto.ParseFromArray(userData.data(), userData.size());
    *responseMessage.mutable_engagement() = userProto.engagement();

    // switch statement for different engagement states
    switch (userProto.engagement().current_action()) {
        case user_engagement::ACTION_NONE:
            HomePageResponseGenerator::BuildHomePageResponse(responseMessage, user, debateWrapper);
            break;
            
        case user_engagement::ACTION_DEBATING:
            DebatePageResponseGenerator::BuildDebatePageResponse(responseMessage, user, userProto, debateWrapper);
            break;
        default:
            // unknown action
            break;
    }

    return responseMessage;
}
