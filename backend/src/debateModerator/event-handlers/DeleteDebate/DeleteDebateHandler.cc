#include "DeleteDebateHandler.h"

#include <iostream>
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"

void DeleteDebateHandler::DeleteDebate(const std::string& root_claim_id, const std::string& user) {
    DebateWrapper debateWrapper;
    std::cout << "[DeleteDebateHandler] DeleteDebate called for user: " << user << ", root claim ID: " << root_claim_id << std::endl;

    debateWrapper.deleteDebate(root_claim_id);
}