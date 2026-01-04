#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class EditClaimHandler {
public:
    static void CancelEditClaim(const int& user_id, DebateWrapper& debateWrapper);
    static void CancelEditClaimDescription(const int& user_id, DebateWrapper& debateWrapper);
    static void StartEditClaim(const int& user_id, DebateWrapper& debateWrapper);
    static void StartEditClaimDescription(const int& user_id, DebateWrapper& debateWrapper);
    static void SubmitEditClaim(const int& user_id, const std::string& newClaim, DebateWrapper& debateWrapper);
    static void SubmitEditClaimDescription(const int& user_id, const std::string& newDescription, DebateWrapper& debateWrapper);
};