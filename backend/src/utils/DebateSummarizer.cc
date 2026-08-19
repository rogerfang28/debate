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
const char* kSummaryPromptTemplate =
    R"PROMPT(You are explaining a structured debate to someone who has never heard of this topic and knows nothing about it.

Assume the reader is intelligent but completely uninformed. They do not know who the people are, what any technical term means, or why this matters. Make them understand the whole disagreement from scratch.

HOW TO READ THE DEBATE BELOW

The debate is a graph, not an essay. Claims are supported and challenged by other claims.

- Indentation means the indented block is offered FOR or AGAINST the line directly above it.
- "SUPPORTED BY" means those claims are the reasons or evidence for the claim above.
- "CHALLENGED BY" means those claims attack the claim above.
- "all of the following TOGETHER" means those claims form ONE argument that needs every part to hold. If one part fails, the whole argument fails. Never present them as separate independent reasons.
- "CHALLENGE AGAINST THE REASONING ABOVE" attacks the logic, not the facts. It accepts the evidence is true but denies it proves the conclusion.
- "accepted" / "refuted" / "open" is where each claim stands. "DISPUTED" means the two sides disagree about that claim.
- Lowercase names are the people arguing.

WRITING RULES - FOLLOW THESE EXACTLY

1. Use simple, everyday words. Explain it the way you would out loud to a smart friend who has no background in this.
2. Keep sentences short. Aim for under 20 words.
3. Explain EVERY term a newcomer would not know, the first time you use it, in plain words in parentheses. This includes scientific terms, legal terms, laws, court doctrines, organisations, and every abbreviation.
   NAME the thing AND explain it. Do not dodge a name to avoid having to explain it - "a government letter" and "a specific lab" leave the reader with nothing they could look up or repeat. Write the name, then explain it.
   The debate often does not say what an organisation is, and you must not invent outside facts - so identify it by the role it plays IN THIS DEBATE. Write "the NIH (the US government body that awarded the research grant)". Write "EcoHealth Alliance (the organisation that got the grant and passed part of it to the Wuhan lab)". Write "the Fifth Amendment (the US legal right to refuse to answer questions that might incriminate you)".
   Key technical terms carry the whole disagreement, so they always get a plain definition on first use: write "gain-of-function research (deliberately changing a virus to give it new or stronger abilities)".

4. Identify each person the first time you name them, by the role the debate shows them playing, for example who is making the accusation and who is answering it. If the debate does not say what someone's job is, do not invent one.
5. If the debate refers to an event, say in a few words what happened at it.
6. Do NOT decide who is right. Never call an argument strong, weak, successful, convincing, damaging or decisive. Report what each side says and where it stands. The reader decides.
7. Add nothing that is not in the debate below. No outside facts, no context you happen to know.
8. Handle each line of argument once, completely, then move on. Never make the same point twice in different places. Do not open with a list of the arguments and then explain them again below.

9. Plain paragraphs only. No markdown, no bullet points, no bold, no numbered lists.

WRITE THE SUMMARY USING THESE EXACT HEADINGS

WHAT THIS IS ABOUT
Two or three simple sentences. What is the main thing being claimed, who is claiming it, and who disputes it. Explain any name or term a newcomer would not know.

THE ARGUMENTS, ONE AT A TIME
Take each separate line of argument in turn, strongest first, and give each its own paragraph. For each one:
 - say what is being claimed, in plain words;
 - say what is offered as support for it;
 - if the debate marked those supports with "all of the following TOGETHER", you MUST say so. This is not optional and it is the easiest thing in the debate to lose. Name the parts and say plainly that the line needs every one of them, so if any single part is wrong the whole line collapses. Attach this only to the argument it belongs to, and name that argument's own parts - never borrow parts from a different argument. Leaving it out makes an argument look sturdier than it is;
 - then say how the other side answers it, and whether they are disputing a fact or accepting the facts while denying they prove the point.
Finish each argument completely before starting the next, so the reader can follow one thread at a time. If a line of argument has drawn no objection, say so.

WHERE IT STANDS NOW
What both sides agree on, what is still unresolved, and the exact point of disagreement. If they are disagreeing about what a word means, name the word and give each side's meaning in plain language.

Aim for 400-550 words total. Do not sacrifice the explanations to stay short - a shorter summary that leaves the reader unable to follow it has failed.

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

WRITE THE SUMMARY USING THESE EXACT HEADINGS

WHAT THIS CLAIM SAYS
Restate the claim in plain words, and say who made it. If it uses any term a newcomer would not know, explain it here.

WHY IT MATTERS HERE
What this claim is being used to support or attack, and what that bigger claim is. If it only works together with other claims, say so plainly, name them, and make clear that the step fails if any one of them fails. If nothing is built on it, say that it stands on its own.

WHAT BACKS IT UP AND WHAT ATTACKS IT
What is offered in support of this claim, and what is offered against it. For each attack, say whether it disputes a fact or accepts the facts while denying they prove the point. If nothing has been offered either way, say so plainly.

WHERE IT STANDS
Whether it is currently accepted, refuted or open, and for whom. If the participants disagree about it, say who takes which view and what the disagreement turns on.

Aim for 150-250 words. Keep it shorter than a whole-debate summary - this is about one claim.

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
