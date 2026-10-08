// should take in debate event protobuf and handle it and give back information
#include "DebateModerator.h"
#include "debate.pb.h"
// #include "debate_list.pb.h"
#include "user.pb.h"
#include "../utils/pathUtils.h"
#include "../moveLog/MoveLogger.h"
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

// modify claim
#include "event-handlers/ModifyClaimHandler/ModifyClaimHandler.h"

#include "event-handlers/EndorseHandler/EndorseHandler.h"
#include "../moveLog/MoveBackfill.h"
#include "event-handlers/ChatHandler/ChatHandler.h"
#include "event-handlers/BriefHandler/BriefHandler.h"
#include "event-handlers/AiAuthorHandler/AiAuthorHandler.h"
#include "event-handlers/TutorialHandler/TutorialHandler.h"

#include "buildResponse/homePageResponse/HomePageResponseGenerator.h"
#include "buildResponse/debatePageResponse/DebatePageResponseGenerator.h"
#include "buildResponse/loginPageResponse/LoginPageResponseGenerator.h"
#include "../utils/Log.h"
#include "../utils/DemoMode.h"
#include "../utils/AnonymousUser.h"

namespace {
bool isViewerBlockedEvent(debate_event::EventType type) {
    switch (type) {
        case debate_event::CREATE_DEBATE:
        case debate_event::CLEAR_DEBATES:
        case debate_event::DELETE_DEBATE:
        case debate_event::LEAVE_DEBATE:
        case debate_event::OPEN_ADD_CHILD_CLAIM:
        case debate_event::CLOSE_ADD_CHILD_CLAIM:
        case debate_event::ADD_CHILD_CLAIM:
        case debate_event::DELETE_CURRENT_STATEMENT:
        case debate_event::DELETE_CHILD_CLAIM:
        case debate_event::START_EDIT_CLAIM_DESCRIPTION:
        case debate_event::SUBMIT_EDIT_CLAIM_DESCRIPTION:
        case debate_event::CANCEL_EDIT_CLAIM_DESCRIPTION:
        case debate_event::START_EDIT_CLAIM:
        case debate_event::SUBMIT_EDIT_CLAIM:
        case debate_event::CANCEL_EDIT_CLAIM:
        case debate_event::CONNECT_FROM_CLAIM:
        case debate_event::CONNECT_TO_CLAIM:
        case debate_event::SUBMIT_CONNECT_CLAIMS:
        case debate_event::CANCEL_CONNECT_CLAIMS:
        case debate_event::DELETE_LINK:
        case debate_event::START_CHALLENGE_CLAIM:
        case debate_event::ADD_CLAIM_TO_BE_CHALLENGED:
        case debate_event::ADD_LINK_TO_BE_CHALLENGED:
        case debate_event::SUBMIT_CHALLENGE_CLAIM:
        case debate_event::CONCEDE_CHALLENGE:
        case debate_event::ENDORSE_CLAIM:
        case debate_event::UNENDORSE_CLAIM:
        case debate_event::CANCEL_CHALLENGE_CLAIM:
        case debate_event::OPEN_ADD_CHALLENGE:
        case debate_event::CLOSE_ADD_CHALLENGE:
        case debate_event::REMOVE_CLAIM_TO_BE_CHALLENGED:
        case debate_event::REMOVE_LINK_TO_BE_CHALLENGED:
        case debate_event::DELETE_CHALLENGE:
        case debate_event::START_MODIFICATION_OF_CLAIM:
        case debate_event::SUBMIT_MODIFICATION_OF_CLAIM:
        case debate_event::CANCEL_MODIFICATION_OF_CLAIM:
            return true;
        default:
            return false;
    }
}

// What an anonymous visitor may not do, on top of everything a read-only
// viewer may not do: join a debate (membership is a change), and the two
// things that write a whole debate (AI generation, a walkthrough -- the
// walkthrough replays a scenario as the viewer, every step of which would be
// blocked anyway). Reading, moving around, the Brief, and the chat stay open:
// none of them changes a debate.
bool isAnonymousBlockedEvent(debate_event::EventType type) {
    if (isViewerBlockedEvent(type)) {
        return true;
    }
    switch (type) {
        case debate_event::JOIN_DEBATE:
        case debate_event::GENERATE_DEBATE:
        case debate_event::START_TUTORIAL:
        case debate_event::TUTORIAL_NEXT:
            return true;
        default:
            return false;
    }
}

bool isClaimMetaSectionBlockedEvent(debate_event::EventType type) {
    switch (type) {
        case debate_event::REPORT_CLAIM:
        case debate_event::GO_TO_HISTORY_OF_CURRENT_CLAIM:
            return true;
        default:
            return false;
    }
}
}

DebateModerator::DebateModerator(Database& usersDb)
    : globalDb(utils::getDatabasePath()),
      dbWrapper(globalDb, usersDb),
      debateWrapper(dbWrapper)
{
    dbWrapper.ensureAllTables();
    // Debates from before the move log get one written from their stored
    // statuses, once, so the page -- which renders from the log -- still
    // shows them. A debate with any move is left alone.
    MoveBackfill::BackfillAll(debateWrapper, dbWrapper);
    Log::info("[DebateModerator] Initialized.");
}

DebateModerator::~DebateModerator() {
    Log::info("[DebateModerator] Destroyed.");
}

// * main function to handle debate event requests
moderator_to_vr::ModeratorToVRMessage DebateModerator::handleRequest(debate_event::DebateEvent& event){
    // Process the debate event and update the database accordingly
    int user_id = event.user().user_id();
    if (!event.user().is_logged_in()) {
        Log::info("[DebateModerator] User not authenticated, redirecting to login page.");
        moderator_to_vr::ModeratorToVRMessage res;
        LoginPageResponseGenerator::BuildLoginPageResponse(res);
        return res;
    }
    Log::info("[DebateModerator] Handling request for user: " + std::to_string(user_id));

    if (demo_mode::kReadOnlyMode && isViewerBlockedEvent(event.type())) {
        Log::info("[DebateModerator] Viewer mode blocked event type: " + std::to_string(event.type()));
        event.set_type(debate_event::NONE);
    }

    // Read from the user record, not the event: user_id is what acts.
    const bool anonymous = anonymous_user::IsAnonymousUsername(dbWrapper.users.getUsername(user_id));
    if (isAnonymousBlockedEvent(event.type()) && anonymous) {
        Log::info("[DebateModerator] Anonymous visitor blocked event type: " + std::to_string(event.type()));
        // Still never acted on as the visitor. A press that starts something
        // (Challenge, Agree, Support, create...) is remembered on the
        // visitor's record and the sign-in modal opens; after a successful
        // sign-in the press is replayed as the signed-in user (see
        // takePendingSignIn).
        if (anonymous_user::OpensSignIn(event.type())) {
            debate_event::DebateEvent remembered = event;
            remembered.clear_user();
            user::User visitor = debateWrapper.getUserProtobuf(user_id);
            visitor.set_sign_in_open(true);
            visitor.set_pending_action(remembered.SerializeAsString());
            debateWrapper.updateUserProtobuf(user_id, visitor);
            Log::info("[DebateModerator] Sign-in requested; remembered event type " +
                      std::to_string(event.type()) + " for visitor " + std::to_string(user_id));
        }
        event.set_type(debate_event::NONE);
    }
    // The header's "Sign in" is a visitor's only: a signed-in user's record
    // never carries the modal (it would greet them again at their next visit).
    if (event.type() == debate_event::OPEN_SIGN_IN && !anonymous) {
        event.set_type(debate_event::NONE);
    }

    if (demo_mode::disableClaimHistoryGuideAndReport && isClaimMetaSectionBlockedEvent(event.type())) {
        Log::info("[DebateModerator] Claim meta UI blocked event type: " + std::to_string(event.type()));
        event.set_type(debate_event::NONE);
    }

    // Move log (parallel record -- nothing reads it yet). Split around the
    // dispatch because an event describes intent while the log records outcome:
    // deletions are only readable before, creations only after. All of the
    // event-to-move mapping lives in MoveLogger; nothing here or in any handler
    // knows what a move is.
    MoveLogger::PreState movePre = MoveLogger::capture(event, user_id, debateWrapper);

    handleDebateEvent(user_id, event);

    MoveLogger::logForEvent(event, user_id, movePre, debateWrapper, dbWrapper);

    moderator_to_vr::ModeratorToVRMessage res = buildResponseMessage(user_id);
    return res;
}

void DebateModerator::handleDebateEvent(const int& user_id, debate_event::DebateEvent& event) {
    // for now we are going to convert to user_id here
    // int user_id = userDb.getUserId(user);
    // Determine the type of event and call the appropriate handler
    switch (event.type()) {
        case debate_event::NONE:
            Log::info("[DebateModerator] Event Type: NONE");
            break;
        case debate_event::CREATE_DEBATE: {
            Log::info("[DebateModerator] Event Type: CREATE_DEBATE");
            debate::Source rootSource;
            rootSource.set_url(event.create_debate().source_url());
            rootSource.set_excerpt(event.create_debate().source_excerpt());
            DebateHandler::AddDebate(event.create_debate().debate_topic(), user_id, debateWrapper,
                                     resolveVoice(event.create_debate().voice()), &rootSource);
            break;
        }
        case debate_event::CLEAR_DEBATES:
            Log::info("[DebateModerator] Event Type: CLEAR_DEBATES");
            DebateHandler::ClearDebates(user_id, debateWrapper);
            break;
        case debate_event::DELETE_DEBATE:
            Log::info("[DebateModerator] Event Type: DELETE_DEBATE");
            DebateHandler::DeleteDebate(event.delete_debate().debate_id(), user_id, debateWrapper);
            break;
        case debate_event::JOIN_DEBATE:
            Log::info("[DebateModerator] Event Type: JOIN_DEBATE");
            DebateHandler::JoinDebateAsMember(event.join_debate().debate_id(), user_id, debateWrapper);
            break;
        case debate_event::ENTER_DEBATE:
            Log::info("[DebateModerator] Event Type: ENTER_DEBATE");
            MoveUserHandler::EnterDebate(event.enter_debate().debate_id(), user_id, debateWrapper);
            break;
        case debate_event::GO_HOME:
            Log::info("[DebateModerator] Event Type: GO_HOME");
            MoveUserHandler::GoHome(user_id, debateWrapper);
            break;
        case debate_event::GO_TO_PARENT:
            Log::info("[DebateModerator] Event Type: GO_TO_PARENT");
            MoveUserHandler::GoToParentClaim(user_id, debateWrapper);
            break;
        case debate_event::GO_TO_CLAIM:
            Log::info("[DebateModerator] Event Type: GO_TO_CLAIM");
            MoveUserHandler::GoToClaim(event.go_to_claim().claim_id(), user_id, debateWrapper);
            break;
        case debate_event::SELECT_CLAIM:
            Log::info("[DebateModerator] Event Type: SELECT_CLAIM");
            MoveUserHandler::SelectClaim(event.select_claim().claim_id(), user_id, debateWrapper);
            break;
        case debate_event::SELECT_LINK:
            Log::info("[DebateModerator] Event Type: SELECT_LINK");
            MoveUserHandler::SelectLink(event.select_link().link_id(), user_id, debateWrapper);
            break;
        case debate_event::ASK_CHAT:
            Log::info("[DebateModerator] Event Type: ASK_CHAT");
            ChatHandler::Ask(event.ask_chat().question(), event.ask_chat().quick_prompt(),
                             user_id, debateWrapper);
            break;
        case debate_event::CLEAR_CHAT:
            Log::info("[DebateModerator] Event Type: CLEAR_CHAT");
            ChatHandler::Clear(user_id, debateWrapper);
            break;
        case debate_event::GO_TO_BRIEF:
            Log::info("[DebateModerator] Event Type: GO_TO_BRIEF");
            BriefHandler::Open(user_id, debateWrapper);
            break;
        case debate_event::GENERATE_BRIEF:
            Log::info("[DebateModerator] Event Type: GENERATE_BRIEF");
            BriefHandler::Generate(user_id, debateWrapper);
            break;
        case debate_event::OPEN_CLAIM_IN_DEBATE:
            Log::info("[DebateModerator] Event Type: OPEN_CLAIM_IN_DEBATE");
            BriefHandler::OpenClaimInDebate(event.select_claim().claim_id(), user_id, debateWrapper);
            break;
        case debate_event::GENERATE_DEBATE:
            Log::info("[DebateModerator] Event Type: GENERATE_DEBATE");
            AiAuthorHandler::GenerateDebate(event.generate_debate().topic(),
                                            event.generate_debate().passage(), user_id,
                                            debateWrapper);
            break;
        case debate_event::CANCEL_AI_WORK:
            Log::info("[DebateModerator] Event Type: CANCEL_AI_WORK");
            AiAuthorHandler::CancelWork(user_id, debateWrapper);
            break;
        case debate_event::START_TUTORIAL:
            Log::info("[DebateModerator] Event Type: START_TUTORIAL");
            TutorialHandler::Start(event.start_tutorial().script(), user_id, *this);
            break;
        case debate_event::TUTORIAL_NEXT:
            Log::info("[DebateModerator] Event Type: TUTORIAL_NEXT");
            TutorialHandler::Next(user_id, *this);
            break;
        case debate_event::TUTORIAL_EXIT:
            Log::info("[DebateModerator] Event Type: TUTORIAL_EXIT");
            TutorialHandler::Exit(user_id, debateWrapper);
            break;
        case debate_event::OPEN_SIGN_IN:
        case debate_event::CLOSE_SIGN_IN: {
            // Opening from the header asks for nothing in particular; closing
            // forgets whatever the visitor was signing in to do.
            const bool open = event.type() == debate_event::OPEN_SIGN_IN;
            Log::info(std::string("[DebateModerator] Event Type: ") + (open ? "OPEN_SIGN_IN" : "CLOSE_SIGN_IN"));
            user::User userProto = debateWrapper.getUserProtobuf(user_id);
            userProto.set_sign_in_open(open);
            userProto.clear_pending_action();
            debateWrapper.updateUserProtobuf(user_id, userProto);
            break;
        }
        case debate_event::DISMISS_AI_NOTICE:
            Log::info("[DebateModerator] Event Type: DISMISS_AI_NOTICE");
            AiAuthorHandler::DismissNotice(user_id, debateWrapper);
            break;
        case debate_event::OPEN_ADD_CHILD_CLAIM:
            Log::info("[DebateModerator] Event Type: OPEN_ADD_CHILD_CLAIM");
            AddClaimHandler::OpenAddChildClaim(user_id, debateWrapper);
            break;
        case debate_event::CLOSE_ADD_CHILD_CLAIM:
            Log::info("[DebateModerator] Event Type: CLOSE_ADD_CHILD_CLAIM");
            AddClaimHandler::CloseAddChildClaim(user_id, debateWrapper);
            break;
        case debate_event::ADD_CHILD_CLAIM:
            Log::info("[DebateModerator] Event Type: ADD_CHILD_CLAIM");
        {
            std::vector<std::string> claimTexts;
            std::vector<std::string> claimDescriptions;
            claimTexts.push_back(event.add_child_claim().claim());
            claimDescriptions.push_back(event.add_child_claim().description());
            for (const auto& extra : event.add_child_claim().extra_claims()) {
                claimTexts.push_back(extra);
            }
            // Padded so the two stay index-aligned even if the client sent
            // fewer descriptions than claims.
            for (const auto& d : event.add_child_claim().extra_descriptions()) {
                claimDescriptions.push_back(d);
            }
            claimDescriptions.resize(claimTexts.size());
            debate::Source source;
            source.set_url(event.add_child_claim().source_url());
            source.set_excerpt(event.add_child_claim().source_excerpt());
            AddClaimHandler::AddClaimUnderClaim(
                claimTexts,
                claimDescriptions,
                event.add_child_claim().basis(),
                user_id,
                debateWrapper,
                resolveVoice(event.add_child_claim().voice()),
                &source
            );
        }
            break;
        case debate_event::REPORT_CLAIM:
            Log::info("[DebateModerator] Event Type: REPORT_CLAIM");
            // implement later
            break;
        case debate_event::DELETE_CURRENT_STATEMENT:
            Log::info("[DebateModerator] Event Type: DELETE_CURRENT_STATEMENT");
            DeleteClaimHandler::DeleteCurrentStatement(user_id, debateWrapper);
            break;
        case debate_event::DELETE_CHILD_CLAIM:
            Log::info("[DebateModerator] Event Type: DELETE_CHILD_CLAIM");
            DeleteClaimHandler::DeleteChildClaim(
                event.delete_child_claim().claim_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::START_EDIT_CLAIM_DESCRIPTION:
            Log::info("[DebateModerator] Event Type: START_EDIT_CLAIM_DESCRIPTION");
            EditClaimHandler::StartEditClaimDescription(user_id, debateWrapper);
            break;
        case debate_event::SUBMIT_EDIT_CLAIM_DESCRIPTION:
            Log::info("[DebateModerator] Event Type: SUBMIT_EDIT_CLAIM_DESCRIPTION");
            EditClaimHandler::SubmitEditClaimDescription(
                user_id,
                event.submit_edit_claim_description().new_description(),
                debateWrapper
            );
            break;
        case debate_event::CANCEL_EDIT_CLAIM_DESCRIPTION:
            Log::info("[DebateModerator] Event Type: CANCEL_EDIT_CLAIM_DESCRIPTION");
            EditClaimHandler::CancelEditClaimDescription(user_id, debateWrapper);
            break;
        case debate_event::START_EDIT_CLAIM:
            Log::info("[DebateModerator] Event Type: START_EDIT_CLAIM");
            EditClaimHandler::StartEditClaim(user_id, debateWrapper);
            break;
        case debate_event::SUBMIT_EDIT_CLAIM:
            Log::info("[DebateModerator] Event Type: SUBMIT_EDIT_CLAIM");
            EditClaimHandler::SubmitEditClaim(
                user_id,
                event.submit_edit_claim().new_claim(),
                debateWrapper
            );
            break;
        case debate_event::CANCEL_EDIT_CLAIM:
            Log::info("[DebateModerator] Event Type: CANCEL_EDIT_CLAIM");
            EditClaimHandler::CancelEditClaim(user_id, debateWrapper);
            break;
        case debate_event::CONNECT_FROM_CLAIM:
            Log::info("[DebateModerator] Event Type: CONNECT_FROM_CLAIM");
            ConnectClaimsHandler::ConnectFromClaim(
                user_id,
                event.connect_from_claim().from_claim_id(),
                debateWrapper
            );
            break;
        case debate_event::CONNECT_TO_CLAIM:
            Log::info("[DebateModerator] Event Type: CONNECT_TO_CLAIM");
            ConnectClaimsHandler::ConnectToClaim(
                user_id,
                event.connect_to_claim().to_claim_id(),
                debateWrapper
            );
            break;
        case debate_event::SUBMIT_CONNECT_CLAIMS:
            Log::info("[DebateModerator] Connection: " + event.submit_connect_claims().connection());
            Log::info("[DebateModerator] Event Type: SUBMIT_CONNECT_CLAIMS");
            ConnectClaimsHandler::ConnectClaims(
                user_id,
                event.submit_connect_claims().connection(),
                debateWrapper
            );
            break; 
        case debate_event::CANCEL_CONNECT_CLAIMS:
            Log::info("[DebateModerator] Event Type: CANCEL_CONNECT_CLAIMS");
            ConnectClaimsHandler::CancelConnectClaims(user_id, debateWrapper);
            break;
        case debate_event::DELETE_LINK:
            Log::info("[DebateModerator] Event Type: DELETE_LINK");
            ConnectClaimsHandler::DeleteLinkById(
                user_id,
                event.delete_link().link_id(),
                debateWrapper
            );
            break;
        // challenges
        case debate_event::START_CHALLENGE_CLAIM:
            Log::info("[DebateModerator] Event Type: START_CHALLENGE_CLAIM");
            ChallengeHandler::StartChallengeClaim(user_id, debateWrapper);
            break;
        case debate_event::ADD_CLAIM_TO_BE_CHALLENGED:
            Log::info("[DebateModerator] Event Type: ADD_CLAIM_TO_BE_CHALLENGED");
            ChallengeHandler::AddClaimToBeChallenged(
                event.add_claim_to_be_challenged().claim_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::ADD_LINK_TO_BE_CHALLENGED:
            Log::info("[DebateModerator] Event Type: ADD_LINK_TO_BE_CHALLENGED");
            ChallengeHandler::AddLinkToBeChallenged(
                event.add_link_to_be_challenged().link_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::SUBMIT_CHALLENGE_CLAIM:
            Log::info("[DebateModerator] Event Type: SUBMIT_CHALLENGE_CLAIM");
        {
            std::vector<std::string> challengeTexts;
            std::vector<std::string> challengeDescriptions;
            challengeTexts.push_back(event.submit_challenge_claim().challenge_sentence());
            challengeDescriptions.push_back(event.submit_challenge_claim().challenge_description());
            for (const auto& extra : event.submit_challenge_claim().extra_sentences()) {
                challengeTexts.push_back(extra);
            }
            for (const auto& d : event.submit_challenge_claim().extra_descriptions()) {
                challengeDescriptions.push_back(d);
            }
            challengeDescriptions.resize(challengeTexts.size());
            debate::Source source;
            source.set_url(event.submit_challenge_claim().source_url());
            source.set_excerpt(event.submit_challenge_claim().source_excerpt());
            ChallengeHandler::SubmitChallengeClaim(
                challengeTexts,
                challengeDescriptions,
                event.submit_challenge_claim().basis(),
                user_id,
                debateWrapper,
                resolveVoice(event.submit_challenge_claim().voice()),
                &source
            );
        }
            break;
        case debate_event::GO_TO_CHALLENGE:
            Log::info("[DebateModerator] Event Type: GO_TO_CHALLENGE");
            MoveUserHandler::GoToChallenge(
                event.go_to_challenge().challenge_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::DELETE_CHALLENGE:
            Log::info("[DebateModerator] Event Type: DELETE_CHALLENGE");
            ChallengeHandler::DeleteChallenge(
                event.delete_challenge().challenge_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::ENDORSE_CLAIM:
            Log::info("[DebateModerator] Event Type: ENDORSE_CLAIM");
            EndorseHandler::Endorse(user_id, event.endorse_claim().claim_id(), debateWrapper);
            break;
        case debate_event::UNENDORSE_CLAIM:
            Log::info("[DebateModerator] Event Type: UNENDORSE_CLAIM");
            EndorseHandler::Unendorse(user_id, event.unendorse_claim().claim_id(), debateWrapper);
            break;
        case debate_event::CONCEDE_CHALLENGE:
            Log::info("[DebateModerator] Event Type: CONCEDE_CHALLENGE");
            {
                int challengeLinkId = event.concede_challenge().challenge_link_id();
                ChallengeHandler::ConcedeChallenge(user_id, challengeLinkId, debateWrapper);
            }
            break;
        case debate_event::CANCEL_CHALLENGE_CLAIM:
            Log::info("[DebateModerator] Event Type: CANCEL_CHALLENGE_CLAIM");
            ChallengeHandler::CancelChallengeClaim(user_id, debateWrapper);
            break;
        case debate_event::OPEN_ADD_CHALLENGE:
            Log::info("[DebateModerator] Event Type: OPEN_ADD_CHALLENGE");
            ChallengeHandler::OpenAddChallenge(user_id, debateWrapper);
            break;
        case debate_event::REMOVE_CLAIM_TO_BE_CHALLENGED:
            Log::info("[DebateModerator] Event Type: REMOVE_CLAIM_TO_BE_CHALLENGED");
            ChallengeHandler::RemoveClaimToBeChallenged(
                event.remove_claim_to_be_challenged().claim_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::REMOVE_LINK_TO_BE_CHALLENGED:
            Log::info("[DebateModerator] Event Type: REMOVE_LINK_TO_BE_CHALLENGED");
            ChallengeHandler::RemoveLinkToBeChallenged(
                event.remove_link_to_be_challenged().link_id(),
                user_id,
                debateWrapper
            );
            break;
        case debate_event::GO_TO_CHALLENGED_PARENT_CLAIM:
            Log::info("[DebateModerator] Event Type: GO_TO_CHALLENGED_PARENT_CLAIM");
            MoveUserHandler::GoToParentClaimOfDebate(user_id, debateWrapper);
            break;
        case debate_event::GO_TO_OVERVIEW:
            Log::info("[DebateModerator] Event Type: GO_TO_OVERVIEW");
            MoveUserHandler::GoToOverview(user_id, debateWrapper);
            break;
        case debate_event::GO_TO_FULL_DEBATE_VIEW:
            Log::info("[DebateModerator] Event Type: GO_TO_FULL_DEBATE_VIEW");
            MoveUserHandler::GoToFullDebateView(user_id, debateWrapper);
            break;
        case debate_event::LEAVE_DEBATE:
            Log::info("[DebateModerator] Event Type: LEAVE_DEBATE");
            DebateHandler::LeaveDebate(
                event.leave_debate().debate_id(),
                user_id,
                debateWrapper,
                dbWrapper
            );
            break;
        case debate_event::START_MODIFICATION_OF_CLAIM:
            Log::info("[DebateModerator] Event Type: START_MODIFICATION_OF_CLAIM");
            ModifyClaimHandler::StartModifyClaim(debateWrapper, user_id);
            break;
        case debate_event::SUBMIT_MODIFICATION_OF_CLAIM:
            Log::info("[DebateModerator] Event Type: SUBMIT_MODIFICATION_OF_CLAIM");
            ModifyClaimHandler::SubmitModifyClaim(debateWrapper, user_id);
            break;
        case debate_event::CANCEL_MODIFICATION_OF_CLAIM:
            Log::info("[DebateModerator] Event Type: CANCEL_MODIFICATION_OF_CLAIM");
            ModifyClaimHandler::CancelModifyClaim(debateWrapper, user_id);
            break;
        default:
            Log::info("[DebateModerator] Event Type: UNKNOWN");
            break;
    }
}

moderator_to_vr::ModeratorToVRMessage DebateModerator::buildResponseMessage(const int& user_id) {
    moderator_to_vr::ModeratorToVRMessage responseMessage;
    // Build the response message based on the current state
    // For example, populate user engagement and debate information
    // so i have to first access the database to get the information about the user engagement
    user::User userProto;

    // std::string user = dbWrapper.users.getUsername(user_id);

    // Fold in any background chat answer BEFORE the user proto is read, so this
    // response carries the current elapsed time or the answer. This is what
    // makes a detached worker visible to the page: the client re-requests
    // while a run is pending, and each request advances what it sees.
    ChatHandler::Refresh(user_id, debateWrapper);
    // The Brief tab's words: a run in flight, or the stored ones for this
    // debate, onto the record the same way.
    BriefHandler::Refresh(user_id, debateWrapper);
    // Generation finishes on a worker; this is where its output becomes
    // claims. It needs the moderator, not just the wrapper, because building
    // goes back through event dispatch.
    AiAuthorHandler::RefreshAiWork(user_id, *this);

    userProto = debateWrapper.getUserProtobuf(user_id);

    // Copy the engagement data
    *responseMessage.mutable_user() = userProto;

    // Strip the credential before it can travel any further. Today this message
    // stays server-side -- MiddleendRequestHandler serialises only the ui::Page
    // to the client, and the frontend decodes nothing but PageSchema -- so this
    // is defence in depth, not a fix for a live leak. It exists so that if this
    // message is ever sent to a client, or logged, or dumped by a debug
    // endpoint, the password hash is already gone. Nothing downstream reads it:
    // verification loads the hash from the database directly
    // (virtualRenderer.cc), never from a response message.
    responseMessage.mutable_user()->clear_password_hash();

    // switch statement for different engagement states
    switch (userProto.engagement().current_action()) {
        case user_engagement::ACTION_HOME:
            Log::info("[DebateModerator] Building HOME page response for user: " + std::to_string(user_id));
            HomePageResponseGenerator::BuildHomePageResponse(responseMessage, user_id, debateWrapper);
            break;
            
        case user_engagement::ACTION_DEBATING:
            Log::info("[DebateModerator] Building DEBATE page response for user: " + std::to_string(user_id));
            DebatePageResponseGenerator::BuildDebatePageResponse(responseMessage, user_id, userProto, debateWrapper);
            break;
        default:
            // unknown action
            Log::info("[DebateModerator] Unknown user engagement action: " + std::to_string(userProto.engagement().current_action()));
            break;
    }

    return responseMessage;
}

// int DebateModerator::validateAuth(debate_event::DebateEvent& event) {
//     int user_id = dbWrapper.users.getUserId(event.user().username());
//     if (!event.user().is_logged_in()) {
//         Log::debug("[DebateModerator] User not authenticated. Check if they are logging in.");
//         if (event.type() == debate_event::LOGIN) {
//             Log::debug("[DebateModerator] Handling LOGIN event.");
//             // check if the user_id is in the database
//             std::string username = event.login().username(); // this means username is unique for now
//             if (user_id != -1) {
//                 Log::debug("[DebateModerator] User " + username + " logged in successfully with user_id: " + std::to_string(user_id));
//                 event.mutable_user()->set_user_id(user_id);
//                 event.mutable_user()->set_is_logged_in(true);
//             } else {
//                 createUserIfNotExist(username);
//                 user_id = dbWrapper.users.getUserId(username);
//             }
//         }
//         else{
//             return -1; // not logged in and not trying to log in
//         }
//     }
//     else {
//         // check if that user_id exists in the database
//         int check_user_id = event.user().user_id();
//         std::string username = event.user().username();
//         if (!dbWrapper.users.userExists(check_user_id)) {
//             Log::debug("[DebateModerator] User ID " + std::to_string(check_user_id) + " not found in database. Redirecting to login page.");
//             createUserIfNotExist(username);
//             user_id = dbWrapper.users.getUserId(username);
//         }
//     }
//     return user_id;
// }

int DebateModerator::resolveVoice(const std::string& name) {
    const size_t first = name.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) {
        return 0;
    }
    const size_t last = name.find_last_not_of(" \t\r\n");
    return createUserIfNotExist(name.substr(first, last - first + 1));
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
        Log::info("[DebateModerator] Created new user: " + username + " with user_id: " + std::to_string(user_id));
    }
    return user_id;
}
// A visitor has just signed in -- as a real account, or as "guest" -- and the
// session now belongs to `userId`. Carries the visitor's place over (which
// page, debate, claim, tab) so the user lands where they were, and takes the
// press they were signing in to make OFF the visitor's record before anything
// runs, so it can only ever run once and never as the visitor. Returns true
// with `pending` filled, as the signed-in user, when there is one to replay.
bool DebateModerator::takePendingSignIn(const int& visitorId, const int& userId,
                                        debate_event::DebateEvent* pending) {
    if (visitorId <= 0 || userId <= 0 || visitorId == userId) {
        return false;
    }
    user::User visitor = debateWrapper.getUserProtobuf(visitorId);
    user::User signedIn = debateWrapper.getUserProtobuf(userId);
    if (!anonymous_user::IsAnonymousUsername(visitor.username()) ||
        anonymous_user::IsAnonymousUsername(signedIn.username())) {
        return false;
    }
    const std::string remembered = visitor.pending_action();

    // The visitor's place, onto the signed-in record. The engagement's AI job
    // and walkthrough stay the user's own; the chat is per person, so the
    // visitor's copy is dropped and the user's own transcript loads instead.
    user_engagement::UserEngagement* engagement = signedIn.mutable_engagement();
    engagement->set_current_action(visitor.engagement().current_action());
    engagement->clear_action_info();
    if (visitor.engagement().has_debating_info()) {
        *engagement->mutable_debating_info() = visitor.engagement().debating_info();
        engagement->mutable_debating_info()->clear_chat();
        engagement->mutable_debating_info()->mutable_brief()->set_pending(false);
    } else if (visitor.engagement().has_home_info()) {
        *engagement->mutable_home_info() = visitor.engagement().home_info();
    }
    *signedIn.mutable_current_scope() = visitor.current_scope();
    *signedIn.mutable_collection_spec() = visitor.collection_spec();
    signedIn.set_sign_in_open(false);
    signedIn.clear_pending_action();

    // The visitor's record forgets the press and the modal, and goes home:
    // signing out later comes back to a visitor on the home page.
    visitor.set_sign_in_open(false);
    visitor.clear_pending_action();
    visitor.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
    visitor.mutable_engagement()->mutable_home_info();
    debateWrapper.updateUserProtobuf(visitorId, visitor);
    debateWrapper.updateUserProtobuf(userId, signedIn);
    Log::info("[DebateModerator] Visitor " + std::to_string(visitorId) + " signed in as user " +
              std::to_string(userId) + " (" + signedIn.username() + "); position carried over.");

    if (remembered.empty() || !pending->ParseFromString(remembered)) {
        return false;
    }
    pending->mutable_user()->set_user_id(userId);
    pending->mutable_user()->set_username(signedIn.username());
    pending->mutable_user()->set_is_logged_in(true);
    Log::info("[DebateModerator] Pending action type " + std::to_string(pending->type()) +
              " taken from visitor " + std::to_string(visitorId) + " for replay as user " +
              std::to_string(userId) + " (" + signedIn.username() + ").");
    return true;
}
