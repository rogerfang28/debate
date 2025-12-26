#pragma once

#include <string>
#include "../../../utils/DebateWrapper.h"

class ClearDebatesHandler {
public:
    static void ClearDebates(const std::string& user, DebateWrapper& debateWrapper);
};