#pragma once

#include "debate_event.pb.h"
#include "../../../utils/DebateWrapper.h"

class AddClaimHandler {
public:
    // `basis` describes how the new claim bears on its parent and is stored on
    // the RELATION, not the claim. Empty means unspecified.
    // Several texts create ONE relation with several from-claims -- "these
    // together support the parent" -- not N independent supports. Blank
    // entries are skipped.
    // `descriptions` is aligned BY INDEX with `claim_texts`; a shorter list or
    // an empty entry simply means that claim has no description. Each claim is
    // its own assertion, so it carries its own explanation.
    // `voice_id`: whose claims these are when not the submitter's (0 = the
    // submitter's own). One voice covers every claim in the call.
    static void AddClaimUnderClaim(const std::vector<std::string>& claim_texts, const std::vector<std::string>& descriptions, const std::string& basis, const int& user_id, DebateWrapper& debateWrapper, int voice_id = 0, const debate::Source* source = nullptr);
    static void CloseAddChildClaim(const int& user_id, DebateWrapper& debateWrapper);
    static void OpenAddChildClaim(const int& user_id, DebateWrapper& debateWrapper);
};