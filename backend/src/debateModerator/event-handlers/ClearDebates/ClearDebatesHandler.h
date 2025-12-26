#pragma once

#include <string>
#include "../../../database/handlers/DebateDatabase.h"
#include "../../../database/handlers/StatementDatabase.h"
#include "../../../database/handlers/UserDatabase.h"
#include "../../../database/handlers/DebateMembersDatabase.h"
#include "../../../utils/DebateWrapper.h"

class ClearDebatesHandler {
public:
    static void ClearDebates(const std::string& user, DebateWrapper& debateWrapper);
};