#pragma once
#include "../../../utils/DebateWrapper.h"
#include <string>

class CancelEditClaim {
public:
    static void Cancel(const std::string& user, DebateWrapper& debateWrapper);
};
