#include "DebateSummarizer.h"

#include "LlmClient.h"
#include "Log.h"

namespace {

// A debate here is not an essay, and summarizing it as one would throw away
// the thing the tool exists to track: what is actually still standing. So the
// prompt asks for the state of play -- what is contested, what is settled,
// what the disagreement turns on -- and explicitly forbids the model from
// judging who is right, which is the user's job, not the summarizer's.
const char* kSummaryPromptTemplate =
    R"PROMPT(You are summarizing a structured debate for someone who needs to catch up on it quickly.

The debate is a graph, not an essay. Claims are supported and challenged by other claims. Read the structure carefully:

- Indentation means the indented block is offered FOR or AGAINST the line directly above it.
- "SUPPORTED BY" means those claims are evidence or argument for the claim above.
- "CHALLENGED BY" means those claims are an attack on the claim above.
- "all of the following TOGETHER" means those claims form ONE joint argument -- it fails entirely if any one of them fails. Do not treat them as separate independent reasons.
- "CHALLENGE AGAINST THE REASONING ABOVE" attacks the INFERENCE, not the claims -- it concedes the evidence but denies that it establishes the conclusion.
- "accepted" / "refuted" / "open" is the current standing of each claim. "DISPUTED" means participants disagree about that claim, which is usually the most important thing on the page.

Write the summary as plain prose with these sections, using these exact headings:

WHAT IS BEING ARGUED
One or two sentences: the central claim and what is at stake.

THE CASE FOR IT
The main lines of support, strongest first. Say what each line actually argues, not just that support exists.

THE CASE AGAINST IT
The challenges raised, and what each one targets -- a specific claim, or the reasoning connecting claims. If nothing has been challenged, say so plainly.

WHERE IT STANDS
What is settled, what is still open, and where participants disagree with each other. Name the specific point the disagreement turns on.

Rules:
- Do NOT decide who is right, and do not add arguments, evidence or facts that are not in the debate below.
- If the debate is thin or one-sided, say that instead of inflating it.
- Refer to claims by their text, not by their id numbers.
- No preamble, no closing remarks, no markdown formatting beyond the headings. Aim for under 300 words.

Here is the debate:

{debate})PROMPT";

} // namespace

std::string DebateSummarizer::BuildPrompt(const std::string& debateText) {
    std::string prompt = kSummaryPromptTemplate;
    const std::string placeholder = "{debate}";
    const size_t pos = prompt.find(placeholder);
    if (pos != std::string::npos) {
        prompt.replace(pos, placeholder.size(), debateText);
    }
    return prompt;
}

std::string DebateSummarizer::Summarize(const std::string& debateText) {
    Log::info("[DebateSummarizer] Summarizing debate (" +
              std::to_string(debateText.size()) + " chars of debate text)");

    LlmOptions options;
    options.label = "debate-summary";
    // Low but not zero: a summary should read the same way twice, and the
    // creative latitude that helps a draft only invites embellishment here.
    options.temperature = 0.2;

    return LlmClient::Complete(BuildPrompt(debateText), options);
}
