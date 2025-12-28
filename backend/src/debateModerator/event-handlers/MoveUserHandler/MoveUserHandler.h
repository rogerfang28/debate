#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class MoveUserHandler {
public:
    static bool EnterDebate(const std::string& debateId, const std::string& user, DebateWrapper& debateWrapper);
    static bool GoHome(const std::string& user, DebateWrapper& debateWrapper);
    static void GoToClaim(const std::string& claim_id, const std::string& user, DebateWrapper& debateWrapper);
    static void GoToParentClaim(const std::string& user, DebateWrapper& debateWrapper);
};
