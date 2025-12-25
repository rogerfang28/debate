#include "DeleteDebateHandler.h"

#include <iostream>
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"

void DeleteDebateHandler::DeleteDebate(const std::string& debate_id, const std::string& user) {
    DebateWrapper debateWrapper;
    std::cout << "[DeleteDebateHandler] DeleteDebate called for user: " << user << ", debate ID: " << debate_id << std::endl;

    debateWrapper.deleteDebate(debate_id, user);
}