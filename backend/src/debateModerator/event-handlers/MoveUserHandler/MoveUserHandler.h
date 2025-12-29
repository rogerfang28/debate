#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class MoveUserHandler {
public:
    static bool EnterDebate(const std::string& debateId, const int& user_id, DebateWrapper& debateWrapper);
    static bool GoHome(const int& user_id, DebateWrapper& debateWrapper);
    static void GoToClaim(const std::string& claim_id, const int& user_id, DebateWrapper& debateWrapper);
    static void GoToParentClaim(const int& user_id, DebateWrapper& debateWrapper);
};
