#pragma once
#include <string>
#include "../../../utils/DebateWrapper.h"

class GoToClaimHandler {
public:
    static void GoToClaim(const std::string& claim_id, const std::string& user, DebateWrapper& debateWrapper);
};