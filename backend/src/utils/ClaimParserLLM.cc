#include "ClaimParserLLM.h"
#include "LlmClient.h"
#include "Log.h"
#include <string>

namespace {

// Endpoint, model and token budget now live in LlmClient, which defaults to the
// same local model this always used.
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

std::string ClaimParserLLM::ExtractClaimsJson(const std::string& articleText) {
    Log::info("[ClaimParserLLM] Extracting claims from article (" +
              std::to_string(articleText.size()) + " chars)");

    LlmOptions options;
    options.label = "claim-extraction";

    // CompleteJson has already proven this parses; what it cannot know is
    // whether the SHAPE is the one this prompt asked for. Callers get that
    // check for free from the schema they read.
    std::string claimsJson = LlmClient::CompleteJson(BuildPrompt(articleText), options);

    Log::info("[ClaimParserLLM] Parsed claim tree from model response");
    return claimsJson;
}
