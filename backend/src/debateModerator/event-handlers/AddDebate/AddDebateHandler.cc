
#include "AddDebateHandler.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../utils/Log.h"

void AddDebateHandler::AddDebate(const std::string& debateTopic, const std::string& user, DebateDatabase& debateDb, StatementDatabase& statementDb, UserDatabase& userDb) {

    Log::debug("[AddDebateHandler] AddDebate called for user: "
              + user + ", topic: " + debateTopic);

    DebateWrapper debateWrapper;

    debateWrapper.initNewDebate(debateTopic, user, debateDb, statementDb, userDb);
}