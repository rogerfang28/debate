#pragma once
#include <string>

class GoToClaimHandler {
public:
    static void GoToClaim(const std::string& claim_id, const std::string& user);
};