// should take in debate event protobuf and handle it and give back information
#include "DebateModerator.h"
#include "../../../src/gen/cpp/debate.pb.h"
// #include "../../../src/gen/cpp/debate_list.pb.h"
#include "../../../src/gen/cpp/user.pb.h"
#include "../utils/pathUtils.h"
#include <iostream>
#include <vector>
// add claim
#include "event-handlers/AddClaimHandler/AddClaimHandler.h"
// debate management
#include "event-handlers/DebateHandler/DebateHandler.h"

// move user
#include "event-handlers/MoveUserHandler/MoveUserHandler.h"
// delete claims
#include "event-handlers/DeleteClaimHandler/DeleteClaimHandler.h"

// edit claims
#include "event-handlers/EditClaimHandler/EditClaimHandler.h"

// connect claims
#include "event-handlers/ConnectClaimsHandler/ConnectClaimsHandler.h"

// challenge
#include "event-handlers/ChallengeHandler/ChallengeHandler.h"

#include "buildResponse/homePageResponse/HomePageResponseGenerator.h"
#include "buildResponse/debatePageResponse/DebatePageResponseGenerator.h"
#include "buildResponse/loginPageResponse/LoginPageResponseGenerator.h"
#include "../utils/Log.h"

DebateModerator::DebateModerator()
    : globalDb(utils::getDatabasePath()),
      dbWrapper(globalDb),
      debateWrapper(dbWrapper)
{
    dbWrapper.ensureAllTables();
    Log::debug("[DebateModerator] Initialized.");
}

DebateModerator::~DebateModerator() {
    Log::debug("[DebateModerator] Destroyed.");
}

// * main function to handle debate event requests
moderator_to_vr::ModeratorToVRMessage DebateModerator::handleRequest(debate_event::DebateEvent& event){
    // Process the debate event and update the database accordingly
    int user_id = event.user().user_id();
    if (!event.user().is_logged_in()) {
        Log::debug("[DebateModerator] User not authenticated, redirecting to login page.");
        moderator_to_vr::ModeratorToVRMessage res;
        LoginPageResponseGenerator::BuildLoginPageResponse(res);
        return res;
    }
    Log::debug("[DebateModerator] Handling request for user: " + std::to_string(user_id));

    handleDebateEvent(user_id, event);
    moderator_to_vr::ModeratorToVRMessage res = buildResponseMessage(user_id);
    return res;
}

void DebateModerator::handleDebateEvent(const int& user_id, debate_event::DebateEvent& event) {
    // for now we are going to convert to user_id here
    // int user_id = userDb.getUserId(user);
    // Determine the type of event and call the appropriate handler
    switch (event.type()) {
        case debate_event::NONE:
            Log::debug("[DebateModerator] Event Type: NONE");
            break;
        case debate_event::CREATE_DEBATE:
            Log::debug("[DebateModerator] Event Type: CREATE_DEBATE");
            DebateHandler::AddDebate(event.create_debate().debate_topic(), user_id, debateWrapper);
            break;
        case debate_event::CLEAR_DEBATES:
            Log::debug("[DebateModerator] Event Type: CLEAR_DEBATES");
            DebateHandler::ClearDebates(user_id, debateWrapper);
            break;
        case debate_event::DELETE_DEBATE:
            Log::debug("[DebateModerator] Event Type: DELETE_DEBATE");
            DebateHandler::DeleteDebate(event.delete_debate().debate_id(), user_id, debateWrapper);
            break;
        case debate_event::JOIN_DEBATE:
            Log::debug("[DebateModerator] Event Type: JOIN_DEBATE");
            DebateHandler::JoinDebateAsMember(event.join_debate().debate_id(), user_id, debateWrapper);
            break;
        case debate_event::ENTER_DEBATE:
            Log::debug("[DebateModerator] Event Type: ENTER_DEBATE");
            MoveUserHandler::EnterDebate(event.enter_debate().debate_id(), user_id, debateWrapper);
            break;
        case debate_event::GO_HOME:
            Log::debug("[DebateModerator] Event Type: GO_HOME");
            MoveUserHandler::GoHome(user_id, debateWrapper);
            break;
        case debate_event::GO_TO_PARENT:
            Log::debug("[DebateModerator] Event Type: GO_TO_PARENT");
            MoveUserHandler::GoToParentClaim(user_id, debateWrapper);
            break;
        case debate_event::GO_TO_CLAIM:
            Log::debug("[DebateModerator] Event Type: GO_TO_CLAIM");
            MoveUserHandler::GoToClaim(event.go_to_claim().claim_id(), user_id, debateWrapper);
            break;
        case debate_event::OPEN_ADD_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: OPEN_ADD_CHILD_CLAIM");
            AddClaimHandler::OpenAddChildClaim(user_id, debateWrapper);
            break;
        case debate_event::CLOSE_ADD_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: CLOSE_ADD_CHILD_CLAIM");
            AddClaimHandler::CloseAddChildClaim(user_id, debateWrapper);
            break;
        case debate_event::ADD_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: ADD_CHILD_CLAIM");
            AddClaimHandler::AddClaimUnderClaim(
                event.add_child_claim().claim(),
                event.add_child_claim().description(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::REPORT_CLAIM:
            Log::debug("[DebateModerator] Event Type: REPORT_CLAIM");
            // implement later
            break;
        case debate_event::DELETE_CURRENT_STATEMENT:
            Log::debug("[DebateModerator] Event Type: DELETE_CURRENT_STATEMENT");
            DeleteClaimHandler::DeleteCurrentStatement(user_id, debateWrapper);
            break;
        case debate_event::DELETE_CHILD_CLAIM:
            Log::debug("[DebateModerator] Event Type: DELETE_CHILD_CLAIM");
            DeleteClaimHandler::DeleteChildClaim(
                event.delete_child_claim().claim_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::START_EDIT_CLAIM_DESCRIPTION:
            Log::debug("[DebateModerator] Event Type: START_EDIT_CLAIM_DESCRIPTION");
            EditClaimHandler::StartEditClaimDescription(user_id, debateWrapper);
            break;
        case debate_event::SUBMIT_EDIT_CLAIM_DESCRIPTION:
            Log::debug("[DebateModerator] Event Type: SUBMIT_EDIT_CLAIM_DESCRIPTION");
            EditClaimHandler::SubmitEditClaimDescription(
                user_id,
                event.submit_edit_claim_description().new_description(),
                debateWrapper
            );
            break;
        case debate_event::CANCEL_EDIT_CLAIM_DESCRIPTION:
            Log::debug("[DebateModerator] Event Type: CANCEL_EDIT_CLAIM_DESCRIPTION");
            EditClaimHandler::CancelEditClaimDescription(user_id, debateWrapper);
            break;
        case debate_event::START_EDIT_CLAIM:
            Log::debug("[DebateModerator] Event Type: START_EDIT_CLAIM");
            EditClaimHandler::StartEditClaim(user_id, debateWrapper);
            break;
        case debate_event::SUBMIT_EDIT_CLAIM:
            Log::debug("[DebateModerator] Event Type: SUBMIT_EDIT_CLAIM");
            EditClaimHandler::SubmitEditClaim(
                user_id,
                event.submit_edit_claim().new_claim(),
                debateWrapper
            );
            break;
        case debate_event::CANCEL_EDIT_CLAIM:
            Log::debug("[DebateModerator] Event Type: CANCEL_EDIT_CLAIM");
            EditClaimHandler::CancelEditClaim(user_id, debateWrapper);
            break;
        case debate_event::CONNECT_FROM_CLAIM:
            Log::debug("[DebateModerator] Event Type: CONNECT_FROM_CLAIM");
            ConnectClaimsHandler::ConnectFromClaim(
                user_id,
                event.connect_from_claim().from_claim_id(),
                debateWrapper
            );
            break;
        case debate_event::CONNECT_TO_CLAIM:
            Log::debug("[DebateModerator] Event Type: CONNECT_TO_CLAIM");
            ConnectClaimsHandler::ConnectToClaim(
                user_id,
                event.connect_to_claim().to_claim_id(),
                debateWrapper
            );
            break;
        case debate_event::SUBMIT_CONNECT_CLAIMS:
            Log::debug("[DebateModerator] Connection: " + event.submit_connect_claims().connection());
            Log::debug("[DebateModerator] Event Type: SUBMIT_CONNECT_CLAIMS");
            ConnectClaimsHandler::ConnectClaims(
                user_id,
                event.submit_connect_claims().connection(),
                debateWrapper
            );
            break; 
        case debate_event::CANCEL_CONNECT_CLAIMS:
            Log::debug("[DebateModerator] Event Type: CANCEL_CONNECT_CLAIMS");
            ConnectClaimsHandler::CancelConnectClaims(user_id, debateWrapper);
            break;
        case debate_event::DELETE_LINK:
            Log::debug("[DebateModerator] Event Type: DELETE_LINK");
            ConnectClaimsHandler::DeleteLinkById(
                user_id,
                event.delete_link().link_id(),
                debateWrapper
            );
            break;
        // challenges
        case debate_event::START_CHALLENGE_CLAIM:
            Log::debug("[DebateModerator] Event Type: START_CHALLENGE_CLAIM");
            ChallengeHandler::StartChallengeClaim(user_id, debateWrapper);
            break;
        case debate_event::ADD_CLAIM_TO_BE_CHALLENGED:
            Log::debug("[DebateModerator] Event Type: ADD_CLAIM_TO_BE_CHALLENGED");
            ChallengeHandler::AddClaimToBeChallenged(
                event.add_claim_to_be_challenged().claim_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::ADD_LINK_TO_BE_CHALLENGED:
            Log::debug("[DebateModerator] Event Type: ADD_LINK_TO_BE_CHALLENGED");
            ChallengeHandler::AddLinkToBeChallenged(
                event.add_link_to_be_challenged().link_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::SUBMIT_CHALLENGE_CLAIM:
            Log::debug("[DebateModerator] Event Type: SUBMIT_CHALLENGE_CLAIM");
            ChallengeHandler::SubmitChallengeClaim(event.submit_challenge_claim().challenge_sentence(),user_id, debateWrapper);
            break;
        case debate_event::GO_TO_CHALLENGE:
            Log::debug("[DebateModerator] Event Type: GO_TO_CHALLENGE");
            MoveUserHandler::GoToChallenge(
                event.go_to_challenge().challenge_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::DELETE_CHALLENGE:
            Log::debug("[DebateModerator] Event Type: DELETE_CHALLENGE");
            ChallengeHandler::DeleteChallenge(
                event.delete_challenge().challenge_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::CONCEDE_CHALLENGE:
            Log::debug("[DebateModerator] Event Type: CONCEDE_CHALLENGE");
            ChallengeHandler::ConcedeChallenge(user_id, debateWrapper);
            break;
        case debate_event::CANCEL_CHALLENGE_CLAIM:
            Log::debug("[DebateModerator] Event Type: CANCEL_CHALLENGE_CLAIM");
            ChallengeHandler::CancelChallengeClaim(user_id, debateWrapper);
            break;
        case debate_event::OPEN_ADD_CHALLENGE:
            Log::debug("[DebateModerator] Event Type: OPEN_ADD_CHALLENGE");
            ChallengeHandler::OpenAddChallenge(user_id, debateWrapper);
            break;
        case debate_event::REMOVE_CLAIM_TO_BE_CHALLENGED:
            Log::debug("[DebateModerator] Event Type: REMOVE_CLAIM_TO_BE_CHALLENGED");
            ChallengeHandler::RemoveClaimToBeChallenged(
                event.remove_claim_to_be_challenged().claim_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::REMOVE_LINK_TO_BE_CHALLENGED:
            Log::debug("[DebateModerator] Event Type: REMOVE_LINK_TO_BE_CHALLENGED");
            ChallengeHandler::RemoveLinkToBeChallenged(
                event.remove_link_to_be_challenged().link_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::GO_TO_CHALLENGED_PARENT_CLAIM:
            Log::debug("[DebateModerator] Event Type: GO_TO_CHALLENGED_PARENT_CLAIM");
            MoveUserHandler::GoToParentClaimOfDebate(user_id, debateWrapper);
            break;
        case debate_event::LEAVE_DEBATE:
            Log::debug("[DebateModerator] Event Type: LEAVE_DEBATE");
            DebateHandler::LeaveDebate(
                event.leave_debate().debate_id(),
                user_id,
                debateWrapper,
                dbWrapper
            );
            break;
        default:
            Log::debug("[DebateModerator] Event Type: UNKNOWN");
            break;
    }
}

moderator_to_vr::ModeratorToVRMessage DebateModerator::buildResponseMessage(const int& user_id) {
    moderator_to_vr::ModeratorToVRMessage responseMessage;
    // Build the response message based on the current state
    // For example, populate user engagement and debate information
    // so i have to first access the database to get the information about the user engagement
    user::User userProto;

    std::string user = dbWrapper.users.getUsername(user_id);
    // int user_id = userDb.getUserId(user);
    
    // now we get info from the database
    userProto = debateWrapper.getUserProtobuf(user_id);
    
    // Set the user data (user_id and username)
    responseMessage.mutable_user()->set_user_id(user_id);
    responseMessage.mutable_user()->set_username(user);
    
    // Copy the engagement data
    *responseMessage.mutable_user()->mutable_engagement() = userProto.engagement();

    // switch statement for different engagement states
    switch (userProto.engagement().current_action()) {
        case user_engagement::ACTION_HOME:
            Log::debug("[DebateModerator] Building HOME page response for user: " + user);
            HomePageResponseGenerator::BuildHomePageResponse(responseMessage, user_id, debateWrapper);
            break;
            
        case user_engagement::ACTION_DEBATING:
            Log::debug("[DebateModerator] Building DEBATE page response for user: " + user);
            DebatePageResponseGenerator::BuildDebatePageResponse(responseMessage, user_id, userProto, debateWrapper);
            break;
        case user_engagement::ACTION_LOGIN:
            Log::debug("[DebateModerator] Building LOGIN page response for user: " + user);
            LoginPageResponseGenerator::BuildLoginPageResponse(responseMessage);
            break;
        default:
            // unknown action
            Log::debug("[DebateModerator] Unknown user engagement action: " + std::to_string(userProto.engagement().current_action()));
            break;
    }

    return responseMessage;
}

int DebateModerator::validateAuth(debate_event::DebateEvent& event) {
    int user_id = dbWrapper.users.getUserId(event.user().username());
    if (!event.user().is_logged_in()) {
        Log::debug("[DebateModerator] User not authenticated. Check if they are logging in.");
        if (event.type() == debate_event::LOGIN) {
            Log::debug("[DebateModerator] Handling LOGIN event.");
            // check if the user_id is in the database
            std::string username = event.login().username(); // this means username is unique for now
            if (user_id != -1) {
                Log::debug("[DebateModerator] User " + username + " logged in successfully with user_id: " + std::to_string(user_id));
                event.mutable_user()->set_user_id(user_id);
                event.mutable_user()->set_is_logged_in(true);
            } else {
                createUserIfNotExist(username);
                user_id = dbWrapper.users.getUserId(username);
            }
        }
        else{
            return -1; // not logged in and not trying to log in
        }
    }
    else {
        // check if that user_id exists in the database
        int check_user_id = event.user().user_id();
        std::string username = event.user().username();
        if (!dbWrapper.users.userExists(check_user_id)) {
            Log::debug("[DebateModerator] User ID " + std::to_string(check_user_id) + " not found in database. Redirecting to login page.");
            createUserIfNotExist(username);
            user_id = dbWrapper.users.getUserId(username);
        }
    }
    return user_id;
}

int DebateModerator::createUserIfNotExist(const std::string& username) {
    int user_id = dbWrapper.users.getUserId(username);
    if (user_id == -1) {
        user::User newUser;
        newUser.set_username(username);
        newUser.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
        // serialize
        std::vector<uint8_t> serializedNewUser(newUser.ByteSizeLong());
        newUser.SerializeToArray(serializedNewUser.data(), serializedNewUser.size());
        user_id = dbWrapper.users.createUser(username, serializedNewUser);
        newUser.set_user_id(user_id);
        serializedNewUser.resize(newUser.ByteSizeLong());
        newUser.SerializeToArray(serializedNewUser.data(), serializedNewUser.size());
        dbWrapper.users.updateUserProtobuf(user_id, serializedNewUser);
        Log::debug("[DebateModerator] Created new user: " + username + " with user_id: " + std::to_string(user_id));
    }
    return user_id;
}