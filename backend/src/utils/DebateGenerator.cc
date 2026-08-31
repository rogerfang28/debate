#include "DebateGenerator.h"

#include "Log.h"

namespace {

// A passage far past this stops being something the model can hold at once,
// and the debate quietly covers only the beginning of it.
constexpr size_t kMaxPassageChars = 15000;

const char* kGeneratePromptTemplate =
    R"PROMPT(You turn a passage into a debate, written as a scenario file.

A debate is a graph: one root claim, and other claims offered FOR or AGAINST it. You will write the sequence of actions that builds that graph.

THE TOPIC:
{topic}

OUTPUT FORMAT

Protobuf text format. Every line is a step. Write ONLY the scenario, no markdown fence, no commentary.

THE ROOT MUST BE AN ASSERTION, NOT A QUESTION. The topic above may be phrased as a question -- "Should cities lower speed limits?" -- and you must turn it into something that can be true or false: "Cities should lower urban speed limits to 20mph". A question cannot be accepted or refuted, so a debate rooted in one has nothing to argue about.

The file always starts exactly like this, with that assertion as the debate topic:

name: "GeneratedDebate"
steps { action { username: "A" event_type: "CREATE_USER" } }
steps { action { username: "A" event_type: "CREATE_DEBATE" debate_topic: "THE ROOT CLAIM" } }
steps { action { username: "A" event_type: "JOIN_DEBATE"  debate_id: 1 } }
steps { action { username: "A" event_type: "ENTER_DEBATE" debate_id: 1 } }

After that, every claim is added by moving to its target and adding under it:

steps { action { username: "A" event_type: "GO_TO_CLAIM" claim_id: 1 } }
steps {
  action {
    username: "A"
    event_type: "ADD_CHILD_CLAIM"
    claim_text: "a claim supporting claim 1"
    claim_description: "optional context, or omit this line"
    basis: "EVIDENTIAL"
  }
}

To attack a claim instead of supporting it, use SUBMIT_CHALLENGE_CLAIM after moving to it:

steps { action { username: "A" event_type: "GO_TO_CLAIM" claim_id: 2 } }
steps {
  action {
    username: "A"
    event_type: "SUBMIT_CHALLENGE_CLAIM"
    claim_text: "a claim arguing against claim 2"
    challenge_description: "optional context, or omit this line"
    basis: "EVIDENTIAL"
  }
}

CLAIM NUMBERING -- THE THING TO GET RIGHT

Claims are numbered in the order you create them, starting at 1.
- Claim 1 is always the root, created by CREATE_DEBATE.
- The first ADD_CHILD_CLAIM or SUBMIT_CHALLENGE_CLAIM makes claim 2, the next makes claim 3, and so on.
- GO_TO_CLAIM must name a claim that ALREADY EXISTS. Never move to a number you have not created yet.
Count carefully as you write. A wrong number attaches an argument to the wrong sentence.

JOINT ARGUMENTS

If several claims are needed TOGETHER -- the point collapses if any one of them is wrong -- put them in ONE action using extra_claim_texts:

steps {
  action {
    username: "A"
    event_type: "ADD_CHILD_CLAIM"
    claim_text: "Supply fell"
    extra_claim_texts: "Demand held steady"
    basis: "EVIDENTIAL"
  }
}

That creates two claims and ONE joint argument. Each extra_claim_texts line is another claim, so count them in the numbering.

Only group when it is really true. Take one claim out: if what remains still gives a reason to believe the target, they are INDEPENDENT and each needs its own action. Separate measurements of the same thing are independent -- revenue rose, turnover fell, sick days fell: remove any one and the rest still argue the case.

RULES FOR THE CLAIMS

1. Every claim is ONE assertion that could be true or false. Split compound sentences.
2. Use only what the passage says. Do not add facts or arguments of your own.
3. Keep the passage's numbers and attributions in the claim text.
4. basis says what KIND of reason the connection is, and picking the right one matters more than it looks:
   EVIDENTIAL   -- rests on facts, measurements, documents, events.
   AUTHORITY    -- rests on who says so: an official body, an expert, a court. Attacks on a source's bias, motive or standing are AUTHORITY too.
   DEFINITIONAL -- rests on what a term means or whether a case falls under a rule or category.
   NORMATIVE    -- rests on values: what ought to be done, or what may legitimately be concluded from something.
   Omit the line only if none fits.
5. Use SUBMIT_CHALLENGE_CLAIM for anything the passage raises against a claim -- objections, limitations, contrary findings. A passage that discusses its own weaknesses should produce challenges.
5b. When two sides read the SAME fact differently -- a study both cite, a document both quote -- do not link the fact straight to a conclusion. State the reading as its own claim ("The letter shows the agency funded the work"), support THAT with the fact, and let the other side's reading challenge the reading. An objection like "selection bias" or "that is not what it admits" attacks a reading, never the fact itself, and it needs a claim to land on.
6. Aim for 8 to 14 claims in total. Work through the WHOLE passage before you stop -- a passage with four distinct arguments should produce four, not two.
7. Attach each claim to the most specific thing it bears on. A rebuttal to a criticism belongs under that criticism, not under the root. Hanging everything off the root loses the shape of the argument.

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
        prompt.replace(topicAt, topicKey.size(),
                       topic.empty() ? "(none given -- take it from the passage)" : topic);
    }
    const std::string passageKey = "{passage}";
    const size_t passageAt = prompt.find(passageKey);
    if (passageAt != std::string::npos) {
        prompt.replace(passageAt, passageKey.size(), truncated);
    }
    return prompt;
}
