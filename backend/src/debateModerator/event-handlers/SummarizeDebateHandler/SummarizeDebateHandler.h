#pragma once

#include "../../../utils/DebateWrapper.h"

class SummarizeDebateHandler {
public:
    // Renders the viewer's whole debate to text, asks the model to summarize
    // it, and stores the result on the viewer for the next page render.
    //
    // Never throws: a model that is unreachable or slow is an ordinary
    // condition, and a failed summary must not take down the request that was
    // only trying to draw a page. The reason is stored and shown instead.
    static void SummarizeDebate(const int& user_id, DebateWrapper& debateWrapper);

    // Clears the stored summary so the panel goes away.
    static void DismissSummary(const int& user_id, DebateWrapper& debateWrapper);

    // Folds the state of any in-flight run into the viewer's stored summary:
    // advances the elapsed counter while it runs, and writes the finished text
    // or error once. Called on every debate page build, which is what turns a
    // background run into something the page can show.
    static void RefreshSummaryState(const int& user_id, DebateWrapper& debateWrapper);

    // Summarizes ONE claim: what it says, what it is holding up, what is
    // offered for and against it, and where it stands.
    static void SummarizeClaim(const int& claim_id, const int& user_id,
                               DebateWrapper& debateWrapper);
    static void DismissClaimSummary(const int& user_id, DebateWrapper& debateWrapper);
};
