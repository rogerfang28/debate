#pragma once
#include "../../../utils/DebateWrapper.h"
#include <string>

class StartEditClaim {
public:
    static void StartEdit(const std::string& user, DebateWrapper& debateWrapper);
};
