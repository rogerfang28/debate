#include "DebateHandler.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"

void DebateHandler::AddDebate(const std::string& debateTopic, const std::string& user, DebateWrapper& debateWrapper) {

    Log::debug("[AddDebateHandler] AddDebate called for user: "
              + user + ", topic: " + debateTopic);

    debateWrapper.initNewDebate(debateTopic, user);
}

void DebateHandler::AddDebate(const std::string& debate_topic, const int& user_id, DebateWrapper& debateWrapper) {
    Log::debug("[AddDebateHandler] AddDebate called for user: " + std::to_string(user_id) + ", topic: " + debate_topic);

    debateWrapper.initNewDebate(debate_topic, user_id);
}

void DebateHandler::DeleteDebate(const std::string& debate_id, const std::string& user, DebateWrapper& debateWrapper) {
    Log::debug("[DeleteDebateHandler] DeleteDebate called for user: " + user + ", debate ID: " + debate_id);

    debateWrapper.deleteDebate(debate_id, user);
}

void DebateHandler::ClearDebates(const std::string& user, DebateWrapper& debateWrapper) {
    Log::debug("[ClearDebatesHandler] ClearDebates called for user: " + user);

    debateWrapper.deleteAllDebates(user);
}

void DebateHandler::JoinDebateAsMember(const std::string& debate_id, const std::string& user, DebateWrapper& debateWrapper) {
    Log::debug("[JoinDebateAsMemberHandler] JoinDebateAsMember called for user: " + user + ", debate ID: " + debate_id);

    // add user as member to debate
    // not implemented yet
    debateWrapper.addMemberToDebate(debate_id, user);
}