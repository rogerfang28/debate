#pragma once

#include "collection.pb.h"
#include <map>
#include <string>

// Renders a whole debate as plain text a language model can read.
//
// The graph is the debate's real shape, but a model reads text, so the job here
// is to flatten a hyperedge graph without losing what makes it an argument:
// which claims support which, which attack which, that several claims can be
// required TOGETHER for one inference, and who accepts what. A flat list of
// sentences would lose all of that and the model would summarize a pile of
// assertions instead of an argument.
//
// Output is stable and indentation-based -- same debate in, same text out --
// so a summary can be compared across runs.
class DebateTextFormatter {
public:
    struct Options {
        // Shown as the debate's subject line.
        std::string topic;
        // Whose acceptances are reported as the baseline. Disagreement with
        // other participants is called out explicitly rather than hidden
        // behind one viewer's view.
        std::string viewerUsername;
    };

    // `usernames` maps creator_id -> display name; unknown ids fall back to
    // "User <id>" rather than being dropped, since who made a move is often
    // the point of the move.
    static std::string Format(const debate::Collection& collection,
                              const std::map<int, std::string>& usernames,
                              const Options& options);
};
