#include "ClearDebatesHandler.h"

#include "../../../utils/pathUtils.h"
#include <iostream>
#include "../../../utils/Log.h"

void ClearDebatesHandler::ClearDebates(const std::string& user, DebateWrapper& debateWrapper) {

    Log::debug("[ClearDebatesHandler] ClearDebates called for user: "
              + user);

    debateWrapper.deleteAllDebates(user);
}