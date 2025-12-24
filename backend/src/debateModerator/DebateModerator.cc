// should take in debate event protobuf and handle it and give back information
#include "DebateModerator.h"
#include "../database/handlers/DebateDatabaseHandler.h"   // now use the new handler
#include "../database/handlers/UserDatabaseHandler.h"
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
#include "event-handlers/AddClaimUnderClaim/AddClaimUnderClaim.h"
#include "buildResponse/homePageResponse/HomePageResponseGenerator.h"
#include "buildResponse/debatePageResponse/DebatePageResponseGenerator.h"
DebateModerator::DebateModerator()
    // : dbHandler(utils::getDatabasePath()) // initialize handler with database
{
    std::cout << "[DebateModerator] Initialized.\n";
}

DebateModerator::~DebateModerator() {
    std::cout << "[DebateModerator] Destroyed.\n";
}

// * main function to handle debate event requests
moderator_to_vr::ModeratorToVRMessage DebateModerator::handleRequest(const std::string& user, debate_event::DebateEvent& event){
    // Process the debate event and update the database accordingly
    std::cout << "[DebateModerator] Handling request for user: " << user << "\n";
    // Example: handle adding a debate topic
    handleDebateEvent(user, event);
    moderator_to_vr::ModeratorToVRMessage res = buildResponseMessage(user);
    return res;
}

void DebateModerator::handleDebateEvent(const std::string& user, debate_event::DebateEvent& event) {
    // Determine the type of event and call the appropriate handler
    switch (event.type()) {
        case debate_event::NONE:
            std::cout << "[DebateModerator] Event Type: NONE\n";
            break;
        case debate_event::CREATE_DEBATE:
            std::cout << "[DebateModerator] Event Type: CREATE_DEBATE\n";
            AddDebateHandler::AddDebate(event.create_debate().debate_topic(), user);
            break;
        case debate_event::CLEAR_DEBATES:
            std::cout << "[DebateModerator] Event Type: CLEAR_DEBATES\n";
            ClearDebatesHandler::ClearDebates(user);
            break;
        case debate_event::DELETE_DEBATE:
            std::cout << "[DebateModerator] Event Type: DELETE_DEBATE\n";
            DeleteDebateHandler::DeleteDebate(event.delete_debate().debate_id(), user);
            break;
        case debate_event::ENTER_DEBATE:
            std::cout << "[DebateModerator] Event Type: ENTER_DEBATE\n";
            EnterDebateHandler::EnterDebate(event.enter_debate().debate_id(), user);
            break;
        case debate_event::GO_HOME:
            std::cout << "[DebateModerator] Event Type: GO_HOME\n";
            GoHomeHandler::GoHome(user);
            break;
        case debate_event::GO_TO_PARENT:
            std::cout << "[DebateModerator] Event Type: GO_TO_PARENT\n";
            // implement later
            break;
        case debate_event::GO_TO_CLAIM:
            std::cout << "[DebateModerator] Event Type: GO_TO_CLAIM\n";
            GoToClaimHandler::GoToClaim(event.go_to_claim().claim_id(), user);
            break;
        case debate_event::OPEN_ADD_CHILD_CLAIM:
            std::cout << "[DebateModerator] Event Type: OPEN_ADD_CHILD_CLAIM\n";
            // implement later
            break;
        case debate_event::ADD_CHILD_CLAIM:
            std::cout << "[DebateModerator] Event Type: ADD_CHILD_CLAIM\n";
            AddClaimUnderClaimHandler::AddClaimUnderClaim(
                event.add_child_claim().claim(),
                event.add_child_claim().connection_to_parent(),
                user
            );
            break;
        case debate_event::REPORT_CLAIM:
            std::cout << "[DebateModerator] Event Type: REPORT_CLAIM\n";
            // implement later
            break;
        case debate_event::DELETE_CURRENT_STATEMENT:
            std::cout << "[DebateModerator] Event Type: DELETE_CURRENT_STATEMENT\n";
            // implement later
            break;
        default:
            std::cout << "[DebateModerator] Event Type: UNKNOWN\n";
            break;
    }
}

moderator_to_vr::ModeratorToVRMessage DebateModerator::buildResponseMessage(const std::string& user) {
    moderator_to_vr::ModeratorToVRMessage responseMessage;
    // Build the response message based on the current state
    // For example, populate user engagement and debate information
    // so i have to first access the database to get the information about the user engagement
    UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    DebateDatabaseHandler debateDbHandler(utils::getDatabasePath());
    user::User userProto;
    
    // user doesn't exist yet, make a default user
    if (!userDbHandler.userExists(user)) {
        userProto.set_username(user);
        userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_NONE);
        userProto.mutable_engagement()->mutable_none_info();

        std::vector<uint8_t> userData(userProto.ByteSizeLong());
        userProto.SerializeToArray(userData.data(), userData.size());
        userDbHandler.addUser(user, userData);
    }

    // now we get info from the database
    std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
    userProto.ParseFromArray(userData.data(), userData.size());
    *responseMessage.mutable_engagement() = userProto.engagement();

    // switch statement for different engagement states
    switch (userProto.engagement().current_action()) {
        case user_engagement::ACTION_NONE:
            HomePageResponseGenerator::BuildHomePageResponse(responseMessage, user);
            break;
            
        case user_engagement::ACTION_DEBATING:
            DebatePageResponseGenerator::BuildDebatePageResponse(responseMessage, user, userProto);
            break;
        default:
            // unknown action
            break;
    }

    return responseMessage;
}
