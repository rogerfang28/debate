// should take in debate event protobuf and handle it and give back information
#include "DebateModerator.h"
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
#include "event-handlers/DeleteCurrentStatement/DeleteCurrentStatement.h"
#include "event-handlers/OpenAddChildClaim/OpenAddChildClaim.h"
#include "event-handlers/CloseAddChildClaim/CloseAddChildClaim.h"
#include "event-handlers/DeleteChildClaim/DeleteChildClaim.h"
#include "event-handlers/StartEditClaimDescription/StartEditClaimDescription.h"
#include "event-handlers/SubmitEditClaimDescription/SubmitEditClaimDescription.h"
#include "event-handlers/CancelEditClaimDescription/CancelEditClaimDescription.h"
#include "event-handlers/StartEditClaim/StartEditClaim.h"
#include "event-handlers/SubmitEditClaim/SubmitEditClaim.h"
#include "event-handlers/CancelEditClaim/CancelEditClaim.h"
#include "event-handlers/ConnectClaimsHandler/ConnectClaimsHandler.h"
#include "buildResponse/homePageResponse/HomePageResponseGenerator.h"
#include "buildResponse/debatePageResponse/DebatePageResponseGenerator.h"
#include "../utils/Log.h"

DebateModerator::DebateModerator()
    : globalDb(utils::getDatabasePath()),
      userDb(globalDb),
      debateDb(globalDb),
      statementDb(globalDb),
      debateMembersDb(globalDb),
      linkDb(globalDb),
      debateWrapper(debateDb, statementDb, userDb, debateMembersDb, linkDb)
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
            GoHomeHandler::GoHome(user, debateWrapper);
            break;
        case debate_event::GO_TO_PARENT:
            Log::debug("[DebateModerator] Event Type: GO_TO_PARENT");
            GoToParentClaimHandler::GoToParentClaim(user, debateWrapper);
            break;
        case debate_event::GO_TO_CLAIM:
            Log::debug("[DebateModerator] Event Type: GO_TO_CLAIM");
            GoToClaimHandler::GoToClaim(event.go_to_claim().claim_id(), user, debateWrapper);
            break;
        case debate_event::OPEN_ADD_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: OPEN_ADD_CHILD_CLAIM");
            OpenAddChildClaimHandler::OpenAddChildClaim(user, debateWrapper);
            break;
        case debate_event::CLOSE_ADD_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: CLOSE_ADD_CHILD_CLAIM");
            CloseAddChildClaimHandler::CloseAddChildClaim(user, debateWrapper);
            break;
        case debate_event::ADD_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: ADD_CHILD_CLAIM");
            AddClaimUnderClaimHandler::AddClaimUnderClaim(
                event.add_child_claim().claim(),
                event.add_child_claim().description(),
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
            DeleteCurrentStatementHandler::DeleteCurrentStatement(user, debateWrapper);
            break;
        case debate_event::DELETE_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: DELETE_CHILD_CLAIM");
            DeleteChildClaimHandler::DeleteChildClaim(
                event.delete_child_claim().claim_id(),
                user,
                debateWrapper
            );
            break;
        case debate_event::START_EDIT_CLAIM_DESCRIPTION:
            Log::debug("[DebateModerator] Event Type: START_EDIT_CLAIM_DESCRIPTION");
            StartEditClaimDescriptionHandler::StartEditClaimDescription(user, debateWrapper);
            break;
        case debate_event::SUBMIT_EDIT_CLAIM_DESCRIPTION:
            Log::debug("[DebateModerator] Event Type: SUBMIT_EDIT_CLAIM_DESCRIPTION");
            SubmitEditClaimDescriptionHandler::SubmitEditClaimDescription(
                user,
                event.submit_edit_claim_description().new_description(),
                debateWrapper
            );
            break;
        case debate_event::CANCEL_EDIT_CLAIM_DESCRIPTION:
            Log::debug("[DebateModerator] Event Type: CANCEL_EDIT_CLAIM_DESCRIPTION");
            CancelEditClaimDescriptionHandler::CancelEditClaimDescription(user, debateWrapper);
            break;
        case debate_event::START_EDIT_CLAIM:
            Log::debug("[DebateModerator] Event Type: START_EDIT_CLAIM");
            StartEditClaim::StartEdit(user, debateWrapper);
            break;
        case debate_event::SUBMIT_EDIT_CLAIM:
            Log::debug("[DebateModerator] Event Type: SUBMIT_EDIT_CLAIM");
            SubmitEditClaim::Submit(
                user,
                event.submit_edit_claim().new_claim(),
                debateWrapper
            );
            break;
        case debate_event::CANCEL_EDIT_CLAIM:
            Log::debug("[DebateModerator] Event Type: CANCEL_EDIT_CLAIM");
            CancelEditClaim::Cancel(user, debateWrapper);
            break;
        case debate_event::CONNECT_FROM_CLAIM:
            Log::debug("[DebateModerator] Event Type: CONNECT_FROM_CLAIM");
            ConnectClaimsHandler::ConnectFromClaim(
                user,
                event.connect_from_claim().from_claim_id(),
                debateWrapper
            );
            break;
        case debate_event::CONNECT_TO_CLAIM:
            Log::debug("[DebateModerator] Event Type: CONNECT_TO_CLAIM");
            ConnectClaimsHandler::ConnectToClaim(
                user,
                event.connect_to_claim().to_claim_id(),
                debateWrapper
            );
            break;
        case debate_event::SUBMIT_CONNECT_CLAIMS:
            Log::debug("[DebateModerator] Event Type: SUBMIT_CONNECT_CLAIMS");
            ConnectClaimsHandler::ConnectClaims(
                user,
                event.submit_connect_claims().connection(),
                debateWrapper
            );
            break; 
        case debate_event::CANCEL_CONNECT_CLAIMS:
            Log::debug("[DebateModerator] Event Type: CANCEL_CONNECT_CLAIMS");
            ConnectClaimsHandler::CancelConnectClaims(user, debateWrapper);
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
    user::User userProto;
    
    // user doesn't exist yet, make a default user
    if (!userDb.userExists(user)) {
        // debateWrapper.createNewUser(user);
        userProto.set_username(user);
        userProto.mutable_engagement()->set_current_action(user_engagement::ACTION_NONE);
        userProto.mutable_engagement()->mutable_none_info();

        std::vector<uint8_t> userData(userProto.ByteSizeLong());
        userProto.SerializeToArray(userData.data(), userData.size());
        int user_id = userDb.createUser(user, userData);
        userProto.set_user_id(std::to_string(user_id));
        std::vector<uint8_t> updatedData(userProto.ByteSizeLong());
        userProto.SerializeToArray(updatedData.data(), updatedData.size());
        userDb.updateUserProtobuf(std::to_string(user_id), updatedData);
        Log::debug("[DebateModerator] Created new user with ID: " + std::to_string(user_id));
    }

    // now we get info from the database
    userProto = debateWrapper.getUserProtobufByUsername(user);
    *responseMessage.mutable_engagement() = userProto.engagement();

    // switch statement for different engagement states
    switch (userProto.engagement().current_action()) {
        case user_engagement::ACTION_NONE:
            Log::debug("[DebateModerator] Building HOME page response for user: " + user);
            HomePageResponseGenerator::BuildHomePageResponse(responseMessage, user, debateWrapper);
            break;
            
        case user_engagement::ACTION_DEBATING:
            Log::debug("[DebateModerator] Building DEBATE page response for user: " + user);
            DebatePageResponseGenerator::BuildDebatePageResponse(responseMessage, user, userProto, debateWrapper);
            break;
        default:
            // unknown action
            Log::debug("[DebateModerator] Unknown user engagement action: " + std::to_string(userProto.engagement().current_action()));
            break;
    }

    return responseMessage;
}
