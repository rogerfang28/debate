#pragma once

#include "../../../../../src/gen/cpp/debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class ChallengeHandler {
public:
    static void StartChallengeClaim(const int& user_id, DebateWrapper& debateWrapper);
    static void CancelChallengeClaim(const int& user_id, DebateWrapper& debateWrapper);
    static void AddClaimToBeChallenged(const int& claim_id, const int& user_id, DebateWrapper& debateWrapper);
    static void AddLinkToBeChallenged(const int& link_id, const int& user_id, DebateWrapper& debateWrapper);
    static void RemoveClaimToBeChallenged(const int& claim_id, const int& user_id, DebateWrapper& debateWrapper);
    static void RemoveLinkToBeChallenged(const int& link_id, const int& user_id, DebateWrapper& debateWrapper);
    static void SubmitChallengeClaim(const std::string& challenge_sentence, const int& user_id, DebateWrapper& debateWrapper);
    static void ConcedeChallenge(const int& user_id, DebateWrapper& debateWrapper);
    static void OpenAddChallenge(const int& user_id, DebateWrapper& debateWrapper);
    static void CloseAddChallenge(const int& user_id, DebateWrapper& debateWrapper);
};