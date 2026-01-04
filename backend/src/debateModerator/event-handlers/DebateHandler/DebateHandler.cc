#include "DebateHandler.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../database/handlers/DatabaseWrapper.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"

void DebateHandler::AddDebate(const std::string& debateTopic, const int& user_id, DebateWrapper& debateWrapper) {

    Log::debug("[AddDebateHandler] AddDebate called for user: "
              + std::to_string(user_id) + ", topic: " + debateTopic);

    debateWrapper.initNewDebate(debateTopic, user_id);
}


void DebateHandler::DeleteDebate(const int& debate_id, const int& user_id, DebateWrapper& debateWrapper) {
    Log::debug("[DeleteDebateHandler] DeleteDebate called for user: " + std::to_string(user_id) + ", debate ID: " + std::to_string(debate_id));

    debateWrapper.deleteDebate(debate_id, user_id);
}

void DebateHandler::ClearDebates(const int& user_id, DebateWrapper& debateWrapper) {
    Log::debug("[ClearDebatesHandler] ClearDebates called for user: " + std::to_string(user_id));

    debateWrapper.deleteAllDebates(user_id);
}

void DebateHandler::JoinDebateAsMember(const int& debate_id, const int& user_id, DebateWrapper& debateWrapper) {
    Log::debug("[JoinDebateAsMemberHandler] JoinDebateAsMember called for user: " + std::to_string(user_id) + ", debate ID: " + std::to_string(debate_id));

    // add user as member to debate
    // not implemented yet
    debateWrapper.addMemberToDebate(debate_id, user_id);
}

void DebateHandler::LeaveDebate(const int& debate_id, const int& user_id, DebateWrapper& debateWrapper, DatabaseWrapper& dbWrapper) {
    Log::debug("[LeaveDebateHandler] LeaveDebate called for user: " + std::to_string(user_id) + ", debate ID: " + std::to_string(debate_id));
    // remove user as member of debate
    dbWrapper.debateMembers.removeMember(debate_id, user_id);
}