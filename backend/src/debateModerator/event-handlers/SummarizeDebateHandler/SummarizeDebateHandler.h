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
};
