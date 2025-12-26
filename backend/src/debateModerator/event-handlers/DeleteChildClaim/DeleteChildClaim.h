#pragma once
#include <string>
#include "../../../utils/DebateWrapper.h"

class DeleteChildClaimHandler {
public:
    static void DeleteChildClaim(const std::string& claimId, const std::string& user, DebateWrapper& debateWrapper);
};