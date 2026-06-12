#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"
    #include "../../../database/virtualrenderer/UserDatabase.h"

class MoveUserHandler {
public:
    static bool EnterDebate(const int& debateId, const int& user_id, DebateWrapper& debateWrapper);
    static bool GoHome(const int& user_id, UserDatabase& userDb);
    static void GoToClaim(const int& claim_id, const int& user_id, DebateWrapper& debateWrapper);
    static void GoToClaim(const int& claim_id, const int& user_id, UserDatabase& userDb);
    static void GoToParentClaim(const int& user_id, DebateWrapper& debateWrapper);
    static void GoToChallenge(const int& claim_id, const int& user_id, UserDatabase& userDb);
    static void GoToParentClaimOfDebate(const int& user_id, DebateWrapper& debateWrapper);
private:
    static void resetOngoingActivities(const int& user_id, UserDatabase& userDb); // user-only reset
    static void resetOngoingActivities(const int& user_id, DebateWrapper& debateWrapper); // like if connecting a claim, stop
};
