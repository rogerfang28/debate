#pragma once
#include <string>
#include "../../../utils/DebateWrapper.h"

class DeleteCurrentStatementHandler {
public:
    static void DeleteCurrentStatement(const std::string& user, DebateWrapper& debateWrapper);
};