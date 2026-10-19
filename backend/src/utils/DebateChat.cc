#include "DebateChat.h"

namespace {

// Lifted from the summary prompts these replaced. Tuned against the Fauci
// debate over several passes; the rules that survived are the ones a real
// failure earned: name a thing AND explain it, keep joint arguments joint,
// never invent an objection, never hand down a verdict.
const char* kRules = R"PROMPT(You are a reader's assistant for ONE structured debate, shown below. You answer questions about it in plain words.

HOW TO READ THE DEBATE

The debate is a graph, not an essay. Claims are supported and challenged by other claims.

- Every claim has a number like [#12]. Use these numbers when you refer to a claim, so the reader can find it.
- Indentation means the indented block is offered FOR or AGAINST the line directly above it.
- "SUPPORTED BY" means those claims are reasons for the claim above. "CHALLENGED BY" means they attack it.
- "all of the following TOGETHER" means those claims are ONE argument needing every part. If one part fails, that whole line fails.
- "CHALLENGE AGAINST THE REASONING ABOVE" accepts the facts but denies they prove the conclusion.
- "accepted" / "refuted" / "open" is where a claim stands. "DISPUTED" means the participants disagree about it.
- "by <name>" is who holds a claim. A name may be a person in the debate, or someone whose words were entered by a reader (a reporter, a witness); either way, it is their claim, not the reader's.
- "source:" is where a claim was taken from. It is not evidence for the claim; evidence is another claim.

RULES

1. Answer the question that was asked. Be brief. Short sentences, everyday words.
2. Explain any term a newcomer would not know, in a few words, the first time you use it. Name a thing AND explain it by the role it plays in this debate.
3. Add nothing that is not in the debate. No outside facts. If the debate does not say, say that it does not say.
4. NEVER INVENT OPPOSITION. Only describe an objection that actually appears under CHALLENGED BY or CHALLENGE AGAINST THE REASONING. If nothing is offered against a claim, say so plainly.
5. Do not decide who is right unless the reader asks you to. Report what is there; the reader decides. If asked for a weakness or a mistake, point to the specific step, name the claims by number, and say what kind of problem it is (a fact that could be wrong, a step that does not follow, a term used two ways, a claim nobody supported).
6. Plain paragraphs. No markdown, no bullet points, no bold, no headings.

THE READER

The person asking is )PROMPT";

}  // namespace

namespace DebateChat {

std::string SystemPrompt(const std::string& debateText, const std::string& viewerName) {
    std::string out = kRules;
    out += viewerName.empty() ? "a reader of this debate." : viewerName + ".";
    out += " When the debate says \"you\", it means them.\n\nTHE DEBATE\n\n";
    out += debateText;
    return out;
}

std::string QuestionFor(const std::string& quickPrompt, int claimId,
                        const std::string& claimSentence) {
    const std::string ref = "[#" + std::to_string(claimId) + "] \"" + claimSentence + "\"";
    if (quickPrompt == kSummarizeDebate) {
        return "Summarize this debate for someone who has never seen it. Say what is being claimed "
               "and who disputes it, take each top-level line of argument in turn with one or two "
               "sentences each, then say what the disagreement really comes down to and what is "
               "still open. Keep it under 180 words, and do not drop a whole line of argument to "
               "stay short.";
    }
    if (quickPrompt == kFindMistake) {
        return "Where is the weakest step in this debate? Point to one specific claim or one "
               "specific step by number, say what kind of problem it is, and say what would fix "
               "it. If you can find no weak step, say so. Do not invent an objection nobody made.";
    }
    if (claimId <= 0) {
        return "";
    }
    if (quickPrompt == kSummarizeClaim) {
        return "Summarize claim " + ref + " for someone who has never seen this debate: what it "
               "says and who holds it, what bigger claim it is offered for or against, what is "
               "offered for and against it, and where it stands. Under 110 words.";
    }
    if (quickPrompt == kSuggestChallenge) {
        return "Suggest one challenge to claim " + ref + ", as a single sentence I could enter as "
               "a claim. Then say in one or two sentences what kind of challenge it is: does it "
               "dispute a fact, deny that the reasoning follows, or question a term. Use only what "
               "is in the debate; if the debate gives you nothing to challenge it with, say so.";
    }
    return "";
}

std::string LabelFor(const std::string& quickPrompt) {
    if (quickPrompt == kSummarizeDebate) return "Summarize this debate";
    if (quickPrompt == kSummarizeClaim) return "Summarize this claim";
    if (quickPrompt == kFindMistake) return "Find a mistake";
    if (quickPrompt == kSuggestChallenge) return "Suggest a challenge to this claim";
    return "";
}

}  // namespace DebateChat
