#pragma once
#include <string>

class AddClaimUnderClaimHandler {
public:
    static void AddClaimUnderClaim(const std::string& claim_text, const std::string& connection_to_parent, const std::string& user);
};