#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class DeleteClaimHandler {
public:
    static void DeleteCurrentStatement(const std::string& user, DebateWrapper& debateWrapper);
    static void DeleteChildClaim(const std::string& claimId, const std::string& user, DebateWrapper& debateWrapper);
};
