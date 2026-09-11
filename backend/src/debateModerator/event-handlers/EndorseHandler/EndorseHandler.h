#pragma once

#include "../../../utils/DebateWrapper.h"

// "I agree."
//
// A reader marks a claim they did not make as TRUE from their own view.
// Nothing about the claim changes for anyone else; the reader's entry in
// the claim's per-user statuses is written, and the graph rules then run
// for them as they do for the author. It is a seed, not a verdict: a
// challenge the reader later holds true still overrides it.
//
// Withdrawing removes the entry. Whatever the graph re-derives for the
// reader afterwards is what they see.
class EndorseHandler {
public:
    // claim_id 0 means the claim the user is standing on.
    static void Endorse(const int& user_id, int claim_id, DebateWrapper& debateWrapper);
    static void Unendorse(const int& user_id, int claim_id, DebateWrapper& debateWrapper);
};
