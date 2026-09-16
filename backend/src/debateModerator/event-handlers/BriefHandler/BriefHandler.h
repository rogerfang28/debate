#pragma once

#include <string>
#include "../../../utils/DebateWrapper.h"

// The Brief tab: the debate as a catch-up page.
//
// The page's structure is computed at render time from the collection (see
// DebateBrief); nothing here stores it. What this handler owns is the model's
// WORDS for that structure -- asked for on demand, written on a worker,
// filed in BRIEFS under the debate and mirrored onto the reader's record so
// the page can draw them -- and the tab itself: opening it, and jumping from
// one of its lines back to the claim on the Debate tab.
class BriefHandler {
public:
    // Opens the Brief tab for the debate the user is in.
    static void Open(const int& user_id, DebateWrapper& debateWrapper);

    // Asks the model for the words. Returns at once; Refresh collects.
    static void Generate(const int& user_id, DebateWrapper& debateWrapper);

    // Folds a running or finished run into the user record, and keeps the
    // record's copy of the stored words current for the debate the user is
    // in. Run on every response.
    static void Refresh(const int& user_id, DebateWrapper& debateWrapper);

    // Back to the Debate tab with this claim selected.
    static void OpenClaimInDebate(const int& claim_id, const int& user_id,
                                  DebateWrapper& debateWrapper);
};
