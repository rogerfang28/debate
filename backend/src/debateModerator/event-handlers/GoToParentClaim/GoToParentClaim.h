#pragma once
#include <string>
#include "../../../utils/DebateWrapper.h"
class GoToParentClaimHandler {
public:
    static void GoToParentClaim(const std::string& user, DebateWrapper& debateWrapper);
};