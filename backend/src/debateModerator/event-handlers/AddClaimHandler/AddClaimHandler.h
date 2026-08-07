#pragma once

#include "debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class AddClaimHandler {
public:
    // `basis` describes how the new claim bears on its parent and is stored on
    // the RELATION, not the claim. Empty means unspecified.
    static void AddClaimUnderClaim(const std::string& claim_text, const std::string& connection_to_parent, const std::string& basis, const int& user_id, DebateWrapper& debateWrapper);
    static void CloseAddChildClaim(const int& user_id, DebateWrapper& debateWrapper);
    static void OpenAddChildClaim(const int& user_id, DebateWrapper& debateWrapper);
};