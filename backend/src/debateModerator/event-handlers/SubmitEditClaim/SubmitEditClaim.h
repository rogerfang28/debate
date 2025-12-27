#pragma once

#include "../../../utils/DebateWrapper.h"
#include <string>

class SubmitEditClaim {
public:
    static void Submit(const std::string& user, const std::string& newClaim, DebateWrapper& debateWrapper);
};
