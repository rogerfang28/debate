#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class EditClaimHandler {
public:
    static void CancelEditClaim(const std::string& user, DebateWrapper& debateWrapper);
    static void CancelEditClaimDescription(const std::string& user, DebateWrapper& debateWrapper);
    static void StartEditClaim(const std::string& user, DebateWrapper& debateWrapper);
    static void StartEditClaimDescription(const std::string& user, DebateWrapper& debateWrapper);
    static void SubmitEditClaim(const std::string& user, const std::string& newClaim, DebateWrapper& debateWrapper);
    static void SubmitEditClaimDescription(const std::string& user, const std::string& newDescription, DebateWrapper& debateWrapper);
};