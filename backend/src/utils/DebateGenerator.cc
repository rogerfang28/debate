#include "DebateGenerator.h"

#include "LlmClient.h"
#include "Log.h"

namespace {

// Truncation guard. A passage far past this stops being something the model
// can hold in mind at once, and the resulting debate quietly covers only the
// beginning of it.
constexpr size_t kMaxPassageChars = 15000;

const char* kGeneratePromptTemplate =
    R"PROMPT(You turn a passage into a structured debate.

A debate here is not an essay. It is a graph: one central claim, and other claims offered FOR or AGAINST it. Your job is to pull that structure out of the passage below.

THE TOPIC THE USER WANTS DEBATED:
{topic}

RULES FOR THE CLAIMS

1. Every claim is ONE assertion that could be true or false. Split compound sentences into separate claims.
2. Use only what the passage says. Do not add facts, figures or arguments from your own knowledge. If the passage does not support a branch, leave it out.
3. Keep the passage's own numbers and attributions in the claim text where they carry the argument.
4. A claim is a sentence, not a heading. "Revenue rose 1.4 percent during the trial" is a claim. "Revenue" is not.
5. The description is optional context for a claim -- where a figure came from, or what it is measuring. Leave it empty if the claim speaks for itself.

RULES FOR THE STRUCTURE

6. Each argument attaches to exactly one target: the root, or a claim introduced by an earlier argument. Never target something that has not been introduced yet.
7. Grouping several claims into one argument means they are JOINT: the point collapses if any single one is wrong. Most claims are NOT joint.
8. Any argument holding more than one claim MUST carry a "joint_reason": one sentence saying what stops working if you remove a single claim from it. If you cannot write that sentence, they are independent -- give each its own argument.
   Separate measurements of the same thing are INDEPENDENT: revenue held up, turnover fell, sick days fell. Remove any one and the rest still argue the case. Separate arguments.
   A fact plus what makes it meaningful is JOINT: "staff worked unpaid on their day off" only bears on the trial if "the trial reported smaller benefits" is also true.
9. Use "challenges" for anything the passage raises AGAINST a claim: objections, limitations, contrary findings, criticisms. A passage that discusses its own weaknesses should produce challenges, not just supports.
10. basis is how the argument bears on its target. One of: EVIDENTIAL (data, studies, observation), AUTHORITY (who said it), DEFINITIONAL (what a term means), NORMATIVE (what ought to be). Use "" if none fits.

OUTPUT

Return ONLY JSON, no markdown fence, in exactly this shape:

{
  "root": {"text": "the central claim the debate is about", "description": ""},
  "arguments": [
    {
      "target": "root",
      "kind": "supports",
      "basis": "EVIDENTIAL",
      "joint_reason": "",
      "claims": [
        {"id": "c1", "text": "a claim supporting the root", "description": ""}
      ]
    },
    {
      "target": "c1",
      "kind": "challenges",
      "basis": "EVIDENTIAL",
      "claims": [
        {"id": "c2", "text": "a claim arguing against c1", "description": ""}
      ]
    }
  ]
}

Every claim id must be unique. Targets must be "root" or an id from an EARLIER argument. Aim for 6 to 14 claims in total, and include at least one challenge if the passage offers any.

THE PASSAGE:

{passage})PROMPT";

} // namespace

std::string DebateGenerator::BuildPrompt(const std::string& topic, const std::string& passage) {
    std::string truncated = passage;
    if (truncated.size() > kMaxPassageChars) {
        truncated = truncated.substr(0, kMaxPassageChars) + "... [truncated]";
    }

    std::string prompt = kGeneratePromptTemplate;
    const std::string topicKey = "{topic}";
    const size_t topicAt = prompt.find(topicKey);
    if (topicAt != std::string::npos) {
        prompt.replace(topicAt, topicKey.size(), topic.empty() ? "(none given -- take it from the passage)" : topic);
    }
    const std::string passageKey = "{passage}";
    const size_t passageAt = prompt.find(passageKey);
    if (passageAt != std::string::npos) {
        prompt.replace(passageAt, passageKey.size(), truncated);
    }
    return prompt;
}

std::string DebateGenerator::Generate(const std::string& topic, const std::string& passage) {
    Log::info("[DebateGenerator] Generating a debate from " + std::to_string(passage.size()) +
              " chars of passage");

    LlmOptions options;
    options.label = "debate-generation";
    // Extraction, not invention: the prompt forbids adding anything the
    // passage does not say, and latitude here is exactly how that gets broken.
    options.temperature = 0.1;

    return LlmClient::CompleteJson(BuildPrompt(topic, passage), options);
}
