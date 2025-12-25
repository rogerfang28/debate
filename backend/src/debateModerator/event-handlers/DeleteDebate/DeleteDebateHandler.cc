#include "DeleteDebateHandler.h"

#include <iostream>
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../utils/Log.h"

void DeleteDebateHandler::DeleteDebate(const std::string& debate_id, const std::string& user) {
    DebateWrapper debateWrapper;
    Log::debug("[DeleteDebateHandler] DeleteDebate called for user: " + user + ", debate ID: " + debate_id);

    debateWrapper.deleteDebate(debate_id, user);
}