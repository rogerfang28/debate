
#include "AddDebateHandler.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"

void AddDebateHandler::AddDebate(const std::string& debateTopic, const std::string& user, DebateWrapper& debateWrapper) {

    Log::debug("[AddDebateHandler] AddDebate called for user: "
              + user + ", topic: " + debateTopic);

    debateWrapper.initNewDebate(debateTopic, user);
}