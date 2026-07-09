#include "ClaimParserLLM.h"
#include "Log.h"
#include "../server/httplib.h"
#include <jsoncons/json.hpp>
#include <sstream>
#include <stdexcept>

namespace {

// Same local LLM endpoint used by the reference claim-tree-viewer prototype.
constexpr const char* kLlmHost = "192.168.86.124";
constexpr int kLlmPort = 8002;
constexpr const char* kLlmPath = "/v1/chat/completions";
constexpr const char* kLlmModel = "Qwen3.6-35B-A3B-UD-Q3_K_XL.gguf";
constexpr int kMaxTokens = 4000;
constexpr size_t kMaxArticleChars = 15000;

// Mirrors claim-tree-viewer/backend/extract_prompt.txt. {article} is
// replaced with the (possibly truncated) input text before sending.
const char* kExtractionPromptTemplate = R"PROMPT(You are a debate tree extractor. Given an article, extract its arguments into a structured debate tree with **supporting** and **challenging** evidence branches.

## Role: Simulated Debate Participant
Imagine you are a knowledgeable user engaging with this article in a debate. Your job is to represent the article's position as accurately as possible, capturing all its claims along with the evidence that SUPPORTS each claim and any CHALLENGING counter-arguments the article acknowledges or addresses.

## Extraction Rules (follow EXACTLY)

### Step 1: Identify the Article's Core Position
Find the article's main thesis or central argument. This is the ROOT of the tree.
- Usually in the intro, summary, or conclusion.
- Write it as a clear, concise one-sentence statement.

### Step 2: Identify the Article's Claims (Branches)
Extract ALL key claims/positions the article makes. Each claim is:
- A distinct argument the article advances.
- Often numbered, bulleted, or in a structured format (FAQ, sections, etc.).
- Must be atomic: each claim = exactly ONE argument.
- Split compound claims ("X and Y") into separate entries.
- Frame each claim from the article's perspective (as if the article is making the argument).

Typical articles have 2-8 claims. Extract ALL of them.

### Step 3: Extract Supporting Evidence (for each claim)
For EACH claim, find the main evidence the article uses to SUPPORT it.
- Write it as a concise summary of the article's main support.

### Step 4: Extract Challenging Evidence (optional, if the article addresses pushback)
If the article acknowledges counter-arguments, rebuttals, or opposing views, include a CHALLENGING branch with the article's response or the challenge itself.
- Write the challenge as the opposing argument the article addresses.
- Then provide the article's rebuttal as evidence, with supporting facts.
- If the article does NOT address any pushback for a claim, omit the challenging section entirely (do NOT invent it).

### Step 5: Extract Supporting Facts
For EACH evidence node (both supporting and challenging), find 1-3 specific supporting facts/data points.
- Never zero facts per evidence -- always include at least 1.
- Prefer specific numbers, studies, statistics, or concrete examples.

## Structural Rules

1. **Each claim gets at least a supporting evidence node.**
2. **Challenging evidence is OPTIONAL** -- only include it if the article actually addresses opposing arguments.
3. **1-3 facts per evidence** -- enough to be informative without clutter.
4. **Atomic claims** -- each claim must express exactly one argument.
5. **Use the article's voice** -- frame claims as the article's position.
6. **Attribution** -- include source name when available.

## Output Format

Return ONLY valid JSON. No markdown wrapping, no explanation. Use this exact schema:

{
  "title": "Article title",
  "source": "Publication name",
  "url": "URL or empty string",
  "core": "Article's main thesis",
  "claims": [
    {
      "text": "The article's claim/position",
      "supporting": {
        "evidence": "Main evidence supporting this claim",
        "facts": ["Specific fact 1", "Specific fact 2"]
      },
      "challenging": {
        "evidence": "The opposing argument the article addresses",
        "counter-evidence": "The article's rebuttal to the challenge",
        "facts": ["Specific fact 1", "Specific fact 2"]
      }
    }
  ]
}

**IMPORTANT:** If a claim has no challenging section in the article, omit the "challenging" field entirely. Only include "challenging" when the article actually addresses opposing arguments.

## Quality Checklist

Before returning, verify:
- Every claim has at least 1 supporting fact.
- Claims are atomic (not compound).
- Supporting evidence directly supports the corresponding claim.
- Challenging sections only included when the article addresses pushback.
- JSON is valid and parseable.

Here is the article:
{article})PROMPT";

// Strips a ```json ... ``` (or bare ``` ... ```) fence if the LLM wrapped
// its JSON output in one, despite being told not to.
std::string StripCodeFence(const std::string& text) {
    size_t start = text.find("```");
    if (start == std::string::npos) {
        return text;
    }
    size_t contentStart = text.find('\n', start);
    if (contentStart == std::string::npos) {
        return text;
    }
    contentStart += 1;
    size_t end = text.find("```", contentStart);
    if (end == std::string::npos) {
        return text;
    }
    return text.substr(contentStart, end - contentStart);
}

} // namespace

std::string ClaimParserLLM::BuildPrompt(const std::string& articleText) {
    std::string truncated = articleText;
    if (truncated.size() > kMaxArticleChars) {
        truncated = truncated.substr(0, kMaxArticleChars) + "... [truncated]";
    }

    std::string prompt = kExtractionPromptTemplate;
    const std::string placeholder = "{article}";
    size_t pos = prompt.find(placeholder);
    if (pos != std::string::npos) {
        prompt.replace(pos, placeholder.size(), truncated);
    }
    return prompt;
}

std::string ClaimParserLLM::CallLLM(const std::string& prompt) {
    jsoncons::json body;
    body["model"] = kLlmModel;
    body["max_tokens"] = kMaxTokens;
    jsoncons::json message;
    message["role"] = "user";
    message["content"] = prompt;
    body["messages"] = jsoncons::json::array();
    body["messages"].push_back(message);

    std::ostringstream bodyStream;
    bodyStream << body;

    httplib::Client cli(kLlmHost, kLlmPort);
    cli.set_connection_timeout(10, 0);
    cli.set_read_timeout(120, 0);

    auto res = cli.Post(kLlmPath, bodyStream.str(), "application/json");
    if (!res) {
        throw std::runtime_error("ClaimParserLLM: request to LLM failed (no response, connection error)");
    }
    if (res->status != 200) {
        throw std::runtime_error("ClaimParserLLM: LLM returned HTTP " + std::to_string(res->status) + ": " + res->body);
    }
    return res->body;
}

std::string ClaimParserLLM::ExtractJsonFromLLMResponse(const std::string& chatCompletionBody) {
    jsoncons::json outer;
    try {
        outer = jsoncons::json::parse(chatCompletionBody);
    } catch (const std::exception& e) {
        throw std::runtime_error("ClaimParserLLM: LLM response envelope wasn't valid JSON: " + std::string(e.what()));
    }

    if (!outer.contains("choices") || outer["choices"].empty()) {
        throw std::runtime_error("ClaimParserLLM: LLM response had no 'choices'");
    }
    std::string content = outer["choices"][0]["message"]["content"].as<std::string>();

    std::string candidate = StripCodeFence(content);

    jsoncons::json parsed;
    try {
        parsed = jsoncons::json::parse(candidate);
    } catch (const std::exception& e) {
        std::string preview = content.substr(0, 200);
        throw std::runtime_error("ClaimParserLLM: LLM did not return valid JSON claims. First 200 chars: " + preview);
    }

    std::ostringstream out;
    out << jsoncons::pretty_print(parsed);
    return out.str();
}

std::string ClaimParserLLM::ExtractClaimsJson(const std::string& articleText) {
    std::string prompt = BuildPrompt(articleText);
    Log::info("[ClaimParserLLM] Sending article (" + std::to_string(articleText.size()) + " chars) to LLM for claim extraction");
    std::string rawResponse = CallLLM(prompt);
    std::string claimsJson = ExtractJsonFromLLMResponse(rawResponse);
    Log::info("[ClaimParserLLM] Successfully parsed claim tree from LLM response");
    return claimsJson;
}
