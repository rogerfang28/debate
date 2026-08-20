#pragma once

#include <string>

// Turns a topic plus a passage into a debate, as JSON.
//
// Owns the generation PROMPT and its schema; the model call belongs to
// LlmClient and turning the JSON into real claims to GeneratedDebateBuilder.
//
// The schema is RELATION-shaped, not tree-shaped: an argument names its
// target, its kind, and the set of claims it introduces. That is the same
// hyperedge the rest of the tool is built on, so a joint argument ("these
// three together") is expressible rather than flattened into a list of
// independent supports on the way in.
class DebateGenerator {
public:
    // Returns validated JSON:
    //   { "root": {...}, "arguments": [ {target, kind, basis, joint_reason, claims[]} ] }
    // Throws std::runtime_error if the model is unreachable or its output is
    // not JSON. Shape beyond that is GeneratedDebateBuilder's problem.
    static std::string Generate(const std::string& topic, const std::string& passage);

    // Exposed for tests and for the prompt-tuning tool.
    static std::string BuildPrompt(const std::string& topic, const std::string& passage);
};
