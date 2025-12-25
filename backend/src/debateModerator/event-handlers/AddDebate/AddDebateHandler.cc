
#include "AddDebateHandler.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"

void AddDebateHandler::AddDebate(const std::string& debateTopic, const std::string& user) {

    std::cout << "[AddDebateHandler] AddDebate called for user: "
              << user << ", topic: " << debateTopic << std::endl;

    // debate::Debate debateProto;
    DebateWrapper debateWrapper;

    debateWrapper.initNewDebate(debateTopic, user);
}