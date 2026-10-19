#pragma once

#include <string>

// Extracts a debate tree (core thesis + claims with supporting/challenging
// evidence) from raw article text.
//
// This owns the claim-extraction PROMPT and nothing else -- the call itself
// belongs to LlmClient, which every AI-backed feature shares. It does not touch
// debate::Collection.
class ClaimParserLLM {
public:
    // Sends articleText to the model with the claim-extraction prompt and
    // returns the parsed TreeData as a validated JSON string:
    //   { "title", "source", "url", "core", "claims": [...] }
    // Throws std::runtime_error if the model is unreachable or its output
    // isn't valid JSON.
    static std::string ExtractClaimsJson(const std::string& articleText);

private:
    static std::string BuildPrompt(const std::string& articleText);
};
