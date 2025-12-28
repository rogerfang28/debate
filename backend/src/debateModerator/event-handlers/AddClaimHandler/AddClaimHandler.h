#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class AddClaimHandler {
public:
    static void AddClaimUnderClaim(const std::string& claim_text, const std::string& connection_to_parent, const std::string& user, DebateWrapper& debateWrapper);
    static void CloseAddChildClaim(const std::string& user, DebateWrapper& debateWrapper);
    static void OpenAddChildClaim(const std::string& user, DebateWrapper& debateWrapper);
};