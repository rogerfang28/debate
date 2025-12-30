#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class DeleteClaimHandler {
public:
    static void DeleteCurrentStatement(const int& user_id, DebateWrapper& debateWrapper);
    static void DeleteChildClaim(const int& claimId, const int& user_id, DebateWrapper& debateWrapper);
};
