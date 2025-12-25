#include "ClearDebatesHandler.h"

#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"
#include <iostream>

void ClearDebatesHandler::ClearDebates(const std::string& user) {

    std::cout << "[ClearDebatesHandler] ClearDebates called for user: "
              << user << std::endl;

    DebateWrapper debateWrapper;
    debateWrapper.deleteAllDebates(user);
}