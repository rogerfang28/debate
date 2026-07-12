#pragma once

#include <string>

// Calls a local LLM to extract a debate tree (core thesis + claims with
// supporting/challenging evidence) from raw article text. This is the
// standalone "call the AI" step -- it does not touch debate::Collection.
class ClaimParserLLM {
public:
    // Sends articleText to the LLM with the claim-extraction prompt and
    // returns the parsed TreeData as a validated JSON string:
    //   { "title", "source", "url", "core", "claims": [...] }
    // Throws std::runtime_error if the LLM is unreachable or its output
    // isn't valid JSON matching the expected shape.
    static std::string ExtractClaimsJson(const std::string& articleText);

private:
    static std::string BuildPrompt(const std::string& articleText);
    static std::string CallLLM(const std::string& prompt);
    static std::string ExtractJsonFromLLMResponse(const std::string& chatCompletionBody);
};
