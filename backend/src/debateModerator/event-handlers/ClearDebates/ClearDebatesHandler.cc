#include "ClearDebatesHandler.h"

#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"
#include <iostream>
#include "../../../utils/Log.h"

void ClearDebatesHandler::ClearDebates(const std::string& user) {

    Log::debug("[ClearDebatesHandler] ClearDebates called for user: "
              + user);

    DebateWrapper debateWrapper;
    debateWrapper.deleteAllDebates(user);
}