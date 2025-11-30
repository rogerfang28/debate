// should take in debate event protobuf and handle it and give back information
#include "DebateModerator.h"
#include "../database/handlers/DebateDatabaseHandler.h"   // now use the new handler
#include "../database/handlers/UserDatabaseHandler.h"
#include "../../../src/gen/cpp/debate.pb.h"
#include "../utils/pathUtils.h"
#include <iostream>
#include <vector>
#include "event-handlers/AddDebate/AddDebateHandler.h"
#include "event-handlers/ClearDebates/ClearDebatesHandler.h"
#include "event-handlers/DeleteDebate/DeleteDebateHandler.h"
#include "event-handlers/EnterDebate/EnterDebateHandler.h"
#include "event-handlers/GoHome/GoHomeHandler.h"
DebateModerator::DebateModerator()
    // : dbHandler(utils::getDatabasePath()) // initialize handler with database
{
    std::cout << "[DebateModerator] Initialized.\n";
}

DebateModerator::~DebateModerator() {
    std::cout << "[DebateModerator] Destroyed.\n";
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
        default:
            std::cout << "[DebateModerator] Event Type: UNKNOWN\n";
            break;
    }
}

moderator_to_vr::ModeratorToVRMessage DebateModerator::buildResponseMessage() {
    moderator_to_vr::ModeratorToVRMessage responseMessage;
    // Build the response message based on the current state
    // For example, populate user engagement and debate information
    // so i have to first access the database to get the information about the user engagement



    return responseMessage;
}

moderator_to_vr::ModeratorToVRMessage DebateModerator::handleRequest(const std::string& user, debate_event::DebateEvent& event){
    // Process the debate event and update the database accordingly
    std::cout << "[DebateModerator] Handling request for user: " << user << "\n";
    // Example: handle adding a debate topic
    handleDebateEvent(user, event);
    moderator_to_vr::ModeratorToVRMessage res = buildResponseMessage();
    return res;
}
