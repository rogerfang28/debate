#include "DebateSummarizer.h"

#include "LlmClient.h"
#include "Log.h"

namespace {

// Tuned against the Fauci debate -- 22 claims, six joint-argument groups, two
// participants who disagree about nearly everything -- with a newcomer as the
// target reader. Seven passes, and most of what is here is scar tissue from a
// specific failure:
//
//   - Naming a term is not explaining it. Early versions printed "NIH" and
//     "gain-of-function" bare, which is fine for someone who already knows and
//     useless for the reader this is written for.
//   - Over-correcting on that made it DODGE names entirely ("a government
//     letter", "a specific lab"), leaving nothing the reader could look up. So
//     the rule is name it AND explain it, using the role the thing plays in
//     this debate -- the debate rarely defines its own terms, and inventing
//     outside facts would be worse than either.
//   - The joint-argument marking is the first thing to fall out of a summary
//     and the most damaging to lose: it is what tells the reader an argument
//     needs every part. Hence the emphatic, non-optional rule.
//   - A worked example naming real claims leaked those claims into unrelated
//     arguments, so the example now demonstrates form, not content.
//   - Separate "case for" and "case against" sections were fought and then
//     abandoned. The model kept answering each argument where it stated it and
//     then repeating itself in the second section. Walking one argument at a
//     time -- claim, support, rebuttal, done -- is both what it does naturally
//     and easier to follow than holding five open threads at once.
//
// Then it was cut to roughly a third. At 550 words it was accurate and nobody
// would read it -- a quick summary that takes two minutes is not a quick
// summary. Shortening by compressing the SAME coverage is what breaks it: the
// model drops the explanations first and leaves a dense page of undefined
// jargon. So the brief version narrows SCOPE instead -- fewer arguments, still
// explained -- and carries one hard exception: the term the disagreement turns
// on always gets defined, because the reader reaching the crux sentence and
// finding gibberish is the one failure that makes the whole summary pointless.
//
// Then the Fauci case -- 22 claims -- showed a fixed word cap is the wrong
// knob. 175 words is right for a three-claim debate and flattens a large one
// into four sentences that drop whole branches. Length now scales with the
// number of top-level lines of argument, and what gets cut when it runs long
// is DETAIL INSIDE a line, never a line. Saying "cover only the biggest
// points" made it worse, not better: it read as licence to drop branches.
//
// The trailing checklist earns its place. An explicit rule plus a worked
// example still left technical terms bare; asking the model to re-read its own
// output and fix them is what actually landed. It does not fix everything --
// organisation names like NIH still come out unexplained more often than not.
const char* kSummaryPromptTemplate =
    R"PROMPT(You are giving someone a QUICK summary of a structured debate. They have never seen this topic and know nothing about it. They want to understand the gist in under a minute, not read an essay.

BE COMPACT, BUT COMPLETE. Someone who wanted the full argument would read the debate itself, so keep it tight -- but a summary that silently leaves out a whole line of argument has failed no matter how short it is. Cut detail, never coverage.

HOW TO READ THE DEBATE BELOW

The debate is a graph, not an essay. Claims are supported and challenged by other claims.

- Indentation means the indented block is offered FOR or AGAINST the line directly above it.
- "SUPPORTED BY" means those claims are reasons for the claim above. "CHALLENGED BY" means they attack it.
- "all of the following TOGETHER" means those claims are ONE argument needing every part. If one part fails, that whole line fails.
- "CHALLENGE AGAINST THE REASONING ABOVE" accepts the facts but denies they prove the conclusion.
- "accepted" / "refuted" / "open" is where a claim stands. "DISPUTED" means the participants disagree about it.
- Lowercase names are the people arguing.

WRITING RULES

1. Simple everyday words. Short sentences, under 20 words.
2. Explain any term a newcomer would not know, in a few words in parentheses, the first time you use it. Name a thing AND explain it - never a bare abbreviation, and never dodge a name to avoid explaining it. Identify things by the role they play in this debate; do not add outside facts.
3. Because you are being brief, mention FEWER things - do not compress by dropping the explanations. It is better to cover two arguments properly than five in a way nobody can follow.
   If a section has little to report, say it in ONE sentence and stop. Never pad a section by restating the same point in different words - on a debate with no objections, "nothing has been challenged" is the complete and correct answer for that section. A short section is a fact about the debate, not a gap to fill.
   If you are running long, cut DETAIL from a line of argument -- never cut the line itself, and never cut the note that its claims stand or fall together.
   Above all: if the disagreement turns on the meaning of a term, that term MUST be explained in plain words. Leaving the crux term undefined is the worst thing this summary can do - the reader reaches the one sentence that matters and it is gibberish to them. If you have no room to explain a term, use plain words instead of the term.
4. Do NOT decide who is right. Never call an argument strong, weak or convincing. The reader decides.
5. Before writing, work out how many distinct top-level lines of argument there are -- the separate branches offered directly for or against the main claim. You must include EVERY one of them. What you skip is detail INSIDE a line: individual pieces of evidence, minor sub-claims, side branches. Never skip a whole line.
6. Add nothing that is not in the debate below.
7. Plain sentences. No markdown, no bullets, no bold.

NEVER INVENT OPPOSITION. Only describe an objection if it actually appears under "CHALLENGED BY" or "CHALLENGE AGAINST THE REASONING ABOVE". If nothing is offered against a claim, say that plainly and stop. Inventing a critic is the worst error you can make here: it tells the reader the argument has been tested when nobody has laid a finger on it, which is the opposite of the truth.

WRITE EXACTLY THESE THREE SHORT SECTIONS

THE QUESTION
One or two sentences. What is being claimed, and who disputes it.

THE ARGUMENT
Take each top-level line of argument in turn, biggest first, about two sentences each. Every line gets a mention, even if only a sentence. If a line needs several claims together, say so in a few words - it matters that the whole line fails if one part does. Then, in one or two sentences, the main objections and whether they dispute the facts or dispute what the facts prove.

WHAT IT COMES DOWN TO
Two to four sentences. The single point the disagreement really turns on. If they are arguing about what a word means, name the word and give both meanings briefly. Then say where things stand: name what both sides accept, and name what is still open or disputed between them. "They disagree about what the facts prove" is not enough on its own -- say which claims.

LENGTH SCALES WITH THE DEBATE. Give each distinct top-level line of argument about two sentences, on top of the two short framing sections. A debate with two lines of argument should land near 120 words; one with five or six lines should land near 300. Never drop a line of argument to hit a number -- coverage first, brevity second. Absolute ceiling 350 words.

A big debate is exactly where a reader needs the structure, so on a long one these are not optional: say which arguments need several claims TOGETHER, and say which claims the two sides actually disagree about rather than only that they disagree.

BEFORE YOU FINISH, RE-READ WHAT YOU WROTE AND FIX THESE:
 - Any abbreviation or organisation name with no explanation next to it? Explain it by the role it plays in this debate.
 - Any technical term a newcomer would not know, used without a plain-words definition?
 - Any line of argument in the debate that you did not mention at all?
 - Any argument marked TOGETHER whose claims you presented as separate reasons?

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

namespace {

// Same plain-language rules as the debate prompt, aimed at a different
// question. The reader has clicked one claim and wants to know what it is
// doing there -- so the ordering is deliberately claim-first: what it says,
// then what it is holding up, then what holds IT up, then whether it survives.
const char* kClaimSummaryPromptTemplate =
    R"PROMPT(You are explaining ONE claim from a structured debate to someone who has never seen this debate and knows nothing about the topic.

Assume the reader is intelligent but completely uninformed. They clicked on this one claim and want to understand what it is and what it is doing in the argument.

HOW TO READ WHAT IS BELOW

- "THE CLAIM IN FOCUS" is the claim the reader selected. Everything else is context around it.
- "WHAT THIS CLAIM IS DOING IN THE DEBATE" shows what the claim is offered in support of, or against. Each step up is a bigger claim that this one helps carry.
- "only works TOGETHER with" means this claim does not carry that bigger claim on its own. It is one part of a joint argument, and if any part fails the whole step fails.
- "WHAT IS OFFERED FOR AND AGAINST THIS CLAIM" lists what supports it and what attacks it.
- "SUPPORTED BY" means reasons or evidence for it. "CHALLENGED BY" means an attack on it.
- "all of the following TOGETHER" under a support or a challenge means those claims are ONE argument, not several. You MUST say so, name the parts, and make clear that the whole line fails if any single part fails. Listing them as separate independent reasons makes the claim look far better supported than it is.
- "CHALLENGE AGAINST THE REASONING ABOVE" attacks the logic rather than the facts: it accepts the evidence but denies it proves the point.
- "accepted" / "refuted" / "open" is where the claim stands. "DISPUTED" means the participants disagree about it.
- Lowercase names are the people arguing.

WRITING RULES - FOLLOW THESE EXACTLY

1. Use simple, everyday words. Explain it the way you would out loud to a smart friend with no background in this.
2. Keep sentences short. Aim for under 20 words.
3. Explain every term a newcomer would not know, the first time you use it, in plain words in parentheses. Name a thing AND explain it - never write a bare abbreviation or organisation name, and never dodge a name to avoid explaining it. The debate rarely defines its own terms and you must not invent outside facts, so identify things by the role they play in this debate.
4. Do NOT decide whether the claim is true or whether an argument is good. Never call anything strong, weak, convincing or damaging. Report what is there. The reader decides.
5. Add nothing that is not below. No outside facts.
6. Plain paragraphs. No markdown, no bullet points, no bold.

BREVITY IS THE POINT. The reader clicked one claim and wants to know what it is, fast. Cover only the biggest points and skip minor detail. Because you are being brief, mention FEWER things rather than compressing by dropping the explanations.

If a section has little to report, say it in ONE sentence and stop. Never pad by restating the same point in different words - "nothing has been offered against it" is a complete answer.

NEVER INVENT OPPOSITION. Only describe an objection if it actually appears under "CHALLENGED BY" or "CHALLENGE AGAINST THE REASONING ABOVE". If nothing is offered against a claim, say that plainly and stop. Inventing a critic is the worst error you can make here: it tells the reader the argument has been tested when nobody has laid a finger on it, which is the opposite of the truth.

WRITE EXACTLY THESE THREE SHORT SECTIONS

WHAT IT SAYS
One or two sentences. The claim in plain words, and who made it. Explain any term a newcomer would not know.

WHAT IT IS DOING HERE
One or two sentences. What bigger claim it supports or attacks. If it only works together with other claims, say so and make clear the step fails if any one of them fails. If nothing is built on it, say it stands on its own.

HOW IT IS HOLDING UP
Two or three sentences. The main things offered for and against it, and whether an objection disputes a fact or accepts the facts while denying they prove the point. Then whether it is accepted, refuted or open, and for whom.

HARD LIMIT: 110 words total. Count them. A summary over 110 words has failed the task.

Here is the claim and its surroundings:

{debate})PROMPT";

} // namespace

std::string DebateSummarizer::SummarizeClaim(const std::string& claimContextText) {
    Log::info("[DebateSummarizer] Summarizing one claim (" +
              std::to_string(claimContextText.size()) + " chars of context)");

    std::string prompt = kClaimSummaryPromptTemplate;
    const std::string placeholder = "{debate}";
    const size_t pos = prompt.find(placeholder);
    if (pos != std::string::npos) {
        prompt.replace(pos, placeholder.size(), claimContextText);
    }

    LlmOptions options;
    options.label = "claim-summary";
    options.temperature = 0.2;
    return LlmClient::Complete(prompt, options);
}
