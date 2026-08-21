#pragma once

#include <string>

// Turns a topic plus a passage into a debate, written as a SCENARIO -- the
// same protobuf text format the test scenarios use.
//
// The model writes user actions rather than a data structure: create the
// debate, go to a claim, add a claim under it, challenge it. Those actions are
// then replayed by scenario::Importer, which is the path the .pbtxt test
// fixtures and the Fauci import already use. Reusing it means generation
// cannot drift from the behaviour those scenarios pin down, and the model is
// writing a format that already has a working reader.
//
// The cost is that claim ids are POSITIONAL: claim 1 is the root, and each
// action creates the next number. The model has to count as it writes, and a
// miscount attaches an argument to the wrong sentence. That is the main thing
// to watch in its output.
class DebateGenerator {
public:
    // The prompt is built on the request thread and handed to a worker, which
    // is why there is no Generate() here: nothing would call it.
    static std::string BuildPrompt(const std::string& topic, const std::string& passage);
};
