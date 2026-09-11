#pragma once

#include "debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class ChallengeHandler {
public:
    static void StartChallengeClaim(const int& user_id, DebateWrapper& debateWrapper);
    static void CancelChallengeClaim(const int& user_id, DebateWrapper& debateWrapper);
    static void AddClaimToBeChallenged(const int& claim_id, const int& user_id, DebateWrapper& debateWrapper);
    static void AddLinkToBeChallenged(const int& link_id, const int& user_id, DebateWrapper& debateWrapper);
    static void RemoveClaimToBeChallenged(const int& claim_id, const int& user_id, DebateWrapper& debateWrapper);
    static void RemoveLinkToBeChallenged(const int& link_id, const int& user_id, DebateWrapper& debateWrapper);
    // Several sentences create ONE opposing relation with several
    // from-claims -- "these together refute that" -- not N independent
    // challenges. `basis` records what KIND of opposition it is and lands on
    // the relation. Blank sentences are skipped.
    // `voice_id`: who is making the challenge when not the submitter (0 =
    // the submitter). The FALSE written against the challenged claim is the
    // voice's opinion, not the submitter's.
    static void SubmitChallengeClaim(const std::vector<std::string>& challenge_sentences, const std::vector<std::string>& descriptions, const std::string& basis, const int& user_id, DebateWrapper& debateWrapper, int voice_id = 0);
    static void ConcedeChallenge(const int& user_id, const int& challenge_link_id, DebateWrapper& debateWrapper);
    static void OpenAddChallenge(const int& user_id, DebateWrapper& debateWrapper);
    static void CloseAddChallenge(const int& user_id, DebateWrapper& debateWrapper);
    static void DeleteChallenge(const int& challenge_id, const int& user_id, DebateWrapper& debateWrapper);
};