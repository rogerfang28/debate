#pragma once

#include <string>
#include <vector>

// Turns a topic plus a passage into a debate, in TWO model calls.
//
// PHASE 1 -- FIND THE CLAIMS. The model reads the passage and lists every
// claim in it, one per line, numbered, with the root assertion as line 1.
// Pure reading; there is almost no syntax to get wrong.
//
// PHASE 2 -- CONNECT THEM. The model is handed that numbered list back and
// says, for each claim, which ONE other claim it bears on, whether it supports
// or opposes it, and what kind of reason it is. One JSON object per line.
//
// Why two calls. Seventeen real runs of the one-call tree format showed the
// model reasoning WELL -- every tree that parsed was three levels deep with
// four or five kinds of basis -- and typing BADLY: 40% of runs died on
// protobuf-text syntax, a missing quote or a misplaced brace, and each fix to
// the prompt was answered with a fresh mistake. Splitting the job puts the
// thinking in a format with no structure to break, and the structure in a
// format where each line stands alone, so one bad line drops one link rather
// than the whole result.
//
// Why the numbers work here when they failed in the step-list format: there
// the model INVENTED ids for claims it was creating and had to track them
// across its own output. Here the ids are printed in front of it and it looks
// them up -- the same thing the AI Challenger already does reliably.
class DebateGenerator {
public:
    // Phase 1.
    static std::string BuildClaimsPrompt(const std::string& topic, const std::string& passage);

    // Phase 2. `claims` is the parsed phase-1 list, in order; claim i is
    // rendered as line i+1. The kinds of reason come from bases::GuideAsList(),
    // so a basis added to backend/config/bases.pbtxt is explained to the model
    // with no code change.
    static std::string BuildConnectionsPrompt(const std::string& topic,
                                              const std::vector<std::string>& claims);

    // The one-call tree prompt this replaced. Kept because ExtractedDebateBuilder
    // still imports hand-written trees and its tests exercise this text; the
    // generation path no longer uses it.
    static std::string BuildPrompt(const std::string& topic, const std::string& passage);
};
