#ifndef DEBATE_CLAIM_PAGE_GENERATOR_H
#define DEBATE_CLAIM_PAGE_GENERATOR_H

#include "../../../../../src/gen/cpp/layout.pb.h"

class DebateClaimPageGenerator {
public:
    static ui::Page GenerateDebatePage(const std::string& claimTitle, const std::string& claimDescription);
};

#endif // DEBATE_CLAIM_PAGE_GENERATOR_H