#pragma once
#include <string>
#include "../../../database/handlers/DebateDatabase.h"
#include "../../../database/handlers/StatementDatabase.h"
#include "../../../database/handlers/UserDatabase.h"
#include "../../../database/handlers/DebateMembersDatabase.h"

class AddDebateHandler {
public:
    static void AddDebate(const std::string& debate_topic, const std::string& user, DebateDatabase& debateDb, StatementDatabase& statementDb, UserDatabase& userDb, DebateMembersDatabase& debateMembersDb);
};