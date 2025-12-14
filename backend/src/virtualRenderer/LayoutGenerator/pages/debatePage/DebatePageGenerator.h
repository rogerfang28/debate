#pragma once
#include "../../../../../src/gen/cpp/layout.pb.h"

class DebatePageGenerator {
public:
    static ui::Page GenerateDebatePage(const std::string& claimTitle, const std::string& claimDescription);
};