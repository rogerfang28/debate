#pragma once

#include <string>

// Turns a rendered debate into a short prose summary, via LlmClient.
//
// Owns the summarization PROMPT and nothing else; the model call belongs to
// LlmClient and the debate-to-text rendering to DebateTextFormatter.
class DebateSummarizer {
public:
    // `debateText` comes from DebateTextFormatter::Format. Returns the summary
    // as plain prose -- not JSON, because the result is shown to a person
    // rather than parsed, and asking for JSON would only add a failure mode.
    // Throws std::runtime_error if the model is unreachable or errors.
    static std::string Summarize(const std::string& debateText);

    // Summarizes ONE claim from its neighbourhood, as produced by
    // DebateTextFormatter::FormatClaimContext. Same audience and the same
    // plain-language rules as the debate summary, but it answers a different
    // question: not "where does this debate stand" but "what is this claim
    // doing here, and is it holding up".
    static std::string SummarizeClaim(const std::string& claimContextText);

    // Exposed for tests: the exact text sent to the model.
    static std::string BuildPrompt(const std::string& debateText);
};
