#pragma once
#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/debate.pb.h"

class DebatePageGenerator {
public:
    static ui::Page GenerateDebatePage(const std::string& claimTitle, const std::string& claimDescription, std::vector<debate::Claim> childClaims);
};