#pragma once

#include <string>

// Turns a topic plus a passage into a debate, as JSON.
//
// Owns the generation PROMPT and its schema; the model call belongs to
// LlmClient and turning the JSON into real claims to GeneratedDebateBuilder.
//
// MEASURED AGAINST GROUND TRUTH. Fed a passage assembled from the Fauci case --
// which exists in the database as a hand-built 22-claim debate -- this prompt
// recovered all four strands (perjury/gain-of-function, the October 2021
// letter, the journals, the Fifth Amendment), each with its supporting evidence
// AND its rebuttal, in 14 claims, with all six joint groups carrying a written
// justification.
//
// It is FLATTER than the hand-built version, and two attempts to fix that made
// things worse, so the flatness stays:
//
//   - The hand-built debate names each strand as its own claim ("his May 2021
//     testimony was false") and hangs the evidence under it. This prompt hangs
//     evidence straight off the root, which misplaces objections: nobody
//     disputes that he SAID it, only that it was false.
//   - A prose rule demanding that intermediate claim did not produce one, and
//     dropped joint_reason compliance from 6/6 to 2/7.
//   - A three-level example in the schema produced depth that was fake -- one
//     node restating the root with everything hanging off it -- and
//     joint_reason went to 0/8, meaning every group would have been split.
//
// Both bought nothing and cost the joint discipline, which matters more: a
// wrongly-joined argument misrepresents how solid a case is, whereas a flat one
// is merely shallow. If you try again, measure joint_reason compliance before
// believing the depth improved.
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
