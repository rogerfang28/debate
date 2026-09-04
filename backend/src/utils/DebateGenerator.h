#pragma once

#include <string>

// Turns a topic plus a passage into a debate, written as a TREE.
//
// The model writes nested claims and arguments -- extraction.ExtractedDebate --
// and ExtractedDebateBuilder walks the tree and creates them.
//
// IT USED TO WRITE A STEP LIST, and that was the wrong shape. A step list made
// the model number its own claims and then navigate to those numbers, holding
// a counter across thirty lines with nothing echoing it back. A captured run
// sent ALL NINE of its navigations to claim 1 and produced no joint arguments
// at all: claim 1 is the only number that is always safe, and a wrong guess
// failed silently, since the file still parsed and still replayed. The result
// was a daisy, not a tree, no matter what the prompt said.
//
// Nesting removes the counter. A claim's parent is the node containing it, so
// depth costs nothing and cannot be got wrong; and a malformed tree is refused
// by the parser instead of importing a quietly wrong graph.
//
// The prompt is a RECURSION rather than a list of rules: take a claim, find
// what the passage offers for and against it, write each as an argument saying
// what kind of reason it is and why -- then do the same for every claim just
// written, until the passage has nothing left to say.
class DebateGenerator {
public:
    // The kinds of reason come from bases::GuideAsList(), so a basis added to
    // backend/config/bases.pbtxt is explained to the model with no code change.
    static std::string BuildPrompt(const std::string& topic, const std::string& passage);
};
