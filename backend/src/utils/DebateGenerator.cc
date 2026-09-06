#include "DebateGenerator.h"

#include <cctype>

#include "BasisRegistry.h"
#include "Log.h"

#include <sstream>

namespace {

// A passage far past this stops being something the model can hold at once,
// and the debate quietly covers only the beginning of it.
constexpr size_t kMaxPassageChars = 15000;

// Replaces every occurrence of `key`.
void substitute(std::string& prompt, const std::string& key, const std::string& value) {
    size_t at = 0;
    while ((at = prompt.find(key, at)) != std::string::npos) {
        prompt.replace(at, key.size(), value);
        at += value.size();
    }
}

std::string truncated(const std::string& passage) {
    if (passage.size() <= kMaxPassageChars) {
        return passage;
    }
    return passage.substr(0, kMaxPassageChars) + "... [truncated]";
}

std::string topicOrHint(const std::string& topic) {
    return topic.empty() ? "(none given -- take it from the passage)" : topic;
}

// ---------------------------------------------------------------------------
// PHASE 1. Reading only. The output is a numbered list, which is about the
// simplest thing a model can be asked to type -- and the compound-sentence and
// no-invention rules live here, where the claims are made.
// ---------------------------------------------------------------------------
const char* kClaimsPromptTemplate =
    R"PROMPT(You read a passage and list every claim it makes about a topic.

THE TOPIC:
{topic}

WHAT COUNTS AS A CLAIM

A claim is ONE assertion that could be true or false. Not a question, not a
topic, not a summary -- a sentence someone could agree or disagree with.

- Split compound sentences. "Deaths fell and journeys got slower" is two claims.
- Include the claims the passage argues AGAINST as well as for. An objection is
  a claim. A reply to an objection is a claim.
- Keep the passage's numbers, dates and attributions inside the claim.
- Use only what the passage says. Never add a fact, a figure or an objection
  the passage does not contain.

LINE 1 IS SPECIAL

Line 1 must be the ROOT: the assertion the whole passage is about, stated so it
can be true or false. If the topic is a question -- "Should cities lower speed
limits?" -- turn it into a statement: "Cities should lower urban speed limits to
20mph". If no topic was given, the root is the passage's central assertion.

FORMAT

One claim per line, numbered from 1. Nothing else: no heading, no commentary,
no blank lines, no grouping, no explanation of why a claim matters.

1. the root claim
2. a claim
3. a claim

Aim for 8 to 20 lines. Work through the WHOLE passage; a passage with four
distinct arguments should produce claims from all four.

THE PASSAGE:

{passage})PROMPT";

// ---------------------------------------------------------------------------
// PHASE 2. Structure only. The claims already exist and are numbered in front
// of the model; it says which bears on which. One JSON object per line, so a
// line that will not parse costs one link, not the result.
// ---------------------------------------------------------------------------
const char* kConnectionsPromptTemplate =
    R"PROMPT(You are given the claims found in a passage, numbered. Say how they connect.

THE TOPIC:
{topic}

THE CLAIMS

{claims}
WHAT A CONNECTION IS

Claim 1 is the root. Every other claim bears on exactly ONE other claim, either
as a reason to believe it (it SUPPORTS it) or as a reason to doubt it (it
OPPOSES it). An OPPOSES connection is a challenge, and a challenged claim can be
challenged in turn.

Connect each claim to the MOST SPECIFIC claim it bears on, not to the root by
default. A reply to an objection connects to the objection. Evidence for a
reason connects to that reason. Only claims that bear on the root directly
connect to claim 1.

HOW TO DECIDE, FOR EACH CLAIM FROM 2 TO THE LAST

  1. Which ONE claim does this bear on most directly?
  2. Is it a reason to believe that claim, or a reason to doubt it?
  3. What KIND of reason is it? Write one line saying why it is that kind.

JOINT REASONS

Sometimes several claims only work TOGETHER -- the point collapses if any one
of them is wrong. Put those in ONE connection with several numbers in "from",
and write joint_reason saying why they cannot be taken apart.

Test before you group. Take one claim out. If what remains is still a reason to
believe the target, they are separate connections. Separate measurements of the
same thing are nearly always separate. A connection with several "from" and no
joint_reason will be split into separate connections.

THE KINDS OF REASON

{bases}
FORMAT

One JSON object per line. Nothing else -- no heading, no commentary, no array
brackets around the lines.

{"from": [3], "to": 1, "role": "supports", "basis": "{default_basis}", "why": "one line: why this is that kind of reason"}
{"from": [5], "to": 3, "role": "opposes", "basis": "{default_basis}", "why": "one line"}
{"from": [7, 8], "to": 1, "role": "supports", "basis": "{default_basis}", "why": "one line", "joint_reason": "why 7 and 8 only work together"}

Every claim from 2 to the last must appear in exactly one "from". Claim 1 never
appears in a "from". Use only the numbers in the list above.

BEFORE YOU FINISH

Count. If the list has 14 claims, there must be a line whose "from" contains 2,
a line whose "from" contains 3, and so on up to 14 -- thirteen placements, no
claim missing, no claim twice. A claim that other claims point at still needs
its own line saying where IT goes.)PROMPT";

// ---------------------------------------------------------------------------
// The one-call tree prompt this replaced. Still used by ExtractedDebateBuilder
// for hand-written trees and by its tests; the generation path no longer runs
// it.
// ---------------------------------------------------------------------------
const char* kTreePromptTemplate =
    R"PROMPT(You read a passage and write down the argument inside it, as a tree.

THE TOPIC:
{topic}

WHAT YOU ARE BUILDING

One root claim, with everything the passage offers for and against it hanging
underneath -- and everything it offers for and against THOSE, and so on.

The root must be an ASSERTION, not a question. The topic above may be phrased
as a question -- "Should cities lower speed limits?" -- and you must turn it
into something that can be true or false: "Cities should lower urban speed
limits to 20mph". A question cannot be accepted or refuted, so a debate rooted
in one has nothing to argue about.

HOW TO BUILD IT -- REPEAT THIS FOR EVERY CLAIM

Starting with the root:

  1. Read the passage again with THIS claim in mind, and only this claim.
  2. Ask what the passage offers as a reason to BELIEVE it, and what it offers
     AGAINST it.
  3. Write each one as an argument under that claim. Say what KIND of reason it
     is, and write one line saying why it is that kind.
  4. Now do steps 1 to 3 for each claim you just wrote.
  5. Stop for a claim when the passage says nothing further about it.

THE FORMAT

Protobuf text format. A claim contains arguments; an argument contains claims.
That nesting IS the structure -- there are no ids anywhere and nothing to
count. A claim's parent is simply the block it sits inside.

root {
  text: "the assertion the passage is about"
  arguments {
    role: SUPPORTS
    basis: "{default_basis}"
    why: "one line: why this is that kind of reason"
    claims {
      text: "a reason the passage gives to believe the claim above"
      arguments {
        role: OPPOSES
        basis: "{default_basis}"
        why: "one line"
        claims { text: "something the passage raises against THAT claim, not against the root" }
      }
    }
  }
}

Several claims inside ONE arguments block are JOINT and need a joint_reason.
Separate arguments blocks are INDEPENDENT.

THE KINDS OF REASON

{bases}
RULES

1. Every claim is ONE assertion that could be true or false.
2. Use only what the passage says.
3. Keep the passage's numbers and attributions inside the claim text.
4. Attach every claim to the MOST SPECIFIC thing it bears on.
5. Write ONLY the tree. No markdown fence, no commentary.

THE PASSAGE:

{passage})PROMPT";

} // namespace

std::string DebateGenerator::RootFromTopic(const std::string& topic) {
    const size_t a = topic.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) {
        return "";
    }
    const size_t b = topic.find_last_not_of(" \t\r\n");
    std::string t = topic.substr(a, b - a + 1);
    if (t.back() == '?') {
        return "";
    }
    // The first word, lower-cased, against the words a question opens with.
    std::string first;
    for (char c : t) {
        if (c == ' ' || c == ',' || c == ':') {
            break;
        }
        first += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    static const char* const kQuestionWords[] = {
        "should", "is", "are", "was", "were", "does", "do", "did", "can", "could",
        "will", "would", "has", "have", "had", "why", "how", "what", "which", "who",
        "whom", "when", "where", "whether", "shall", "may", "might", "must"};
    for (const char* w : kQuestionWords) {
        if (first == w) {
            return "";
        }
    }
    return t;
}

std::string DebateGenerator::BuildClaimsPrompt(const std::string& topic, const std::string& passage) {
    std::string prompt = kClaimsPromptTemplate;
    substitute(prompt, "{topic}", topicOrHint(topic));
    // Last, so a passage that happens to contain a placeholder is not itself
    // substituted into.
    substitute(prompt, "{passage}", truncated(passage));
    return prompt;
}

std::string DebateGenerator::BuildConnectionsPrompt(const std::string& topic,
                                                    const std::vector<std::string>& claims) {
    std::ostringstream numbered;
    for (size_t i = 0; i < claims.size(); ++i) {
        numbered << (i + 1) << ". " << claims[i] << "\n";
    }
    std::string prompt = kConnectionsPromptTemplate;
    substitute(prompt, "{topic}", topicOrHint(topic));
    substitute(prompt, "{bases}", bases::GuideAsList());
    substitute(prompt, "{default_basis}", bases::DefaultName());
    substitute(prompt, "{claims}", numbered.str());
    return prompt;
}

std::string DebateGenerator::BuildPrompt(const std::string& topic, const std::string& passage) {
    std::string prompt = kTreePromptTemplate;
    substitute(prompt, "{topic}", topicOrHint(topic));
    substitute(prompt, "{bases}", bases::GuideAsList());
    substitute(prompt, "{default_basis}", bases::DefaultName());
    substitute(prompt, "{passage}", truncated(passage));
    return prompt;
}
