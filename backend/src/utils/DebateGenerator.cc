#include "DebateGenerator.h"

#include "BasisRegistry.h"
#include "Log.h"

namespace {

// A passage far past this stops being something the model can hold at once,
// and the debate quietly covers only the beginning of it.
constexpr size_t kMaxPassageChars = 15000;

const char* kGeneratePromptTemplate =
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

You are finished when no claim has anything left to say about it. Do not stop
early because the tree looks big enough, and do not keep going by inventing
material the passage does not contain.

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
      description: "context worth keeping, or omit this line"
      arguments {
        role: OPPOSES
        basis: "{default_basis}"
        why: "one line"
        claims {
          text: "something the passage raises against THAT claim, not against the root"
        }
      }
    }
  }
}

role is SUPPORTS or OPPOSES. An OPPOSES argument is a challenge, and a
challenge is an ordinary claim: it can be supported and challenged in turn, so
keep nesting under it exactly as you would anywhere else.

ONE ARGUMENT OR SEVERAL -- THE THING TO GET RIGHT

Several claims inside ONE arguments block are JOINT: the argument dies if any
one of them is wrong. Two separate arguments blocks under the same claim are
INDEPENDENT: each stands on its own, and losing one leaves the other standing.

Test it before you group. Take one claim out of the group. If what remains is
still a reason to believe the parent, they are INDEPENDENT and each needs its
own arguments block. Separate measurements of the same thing are nearly always
independent -- revenue rose, sick days fell, turnover fell: remove any one and
the rest still argue the case.

If you do group, you must write joint_reason saying why they cannot be taken
apart. A group with no joint_reason will be split into independent arguments.

THE KINDS OF REASON

{bases}
RULES

1. Every claim is ONE assertion that could be true or false. Split compound
   sentences into separate claims.
2. Use only what the passage says. Never invent a fact, a study, a figure or an
   objection. If the passage raises no objection to a claim, it has none.
3. Keep the passage's numbers and attributions inside the claim text.
4. Attach every claim to the MOST SPECIFIC thing it bears on. A rebuttal to a
   criticism goes under that criticism, not under the root.
5. When two sides read the SAME fact differently -- a study both cite, a
   document both quote -- do not hang the fact straight under the conclusion.
   Write the READING as its own claim, support that reading with the fact, and
   let the other side's reading OPPOSE the reading. An objection like "selection
   bias" or "that is not what it admits" attacks a reading, never the fact.
6. Aim for 8 to 20 claims, and work through the WHOLE passage. A passage with
   four distinct arguments should produce four, not two.
7. Write ONLY the tree. No markdown fence, no commentary before or after.

THE PASSAGE:

{passage})PROMPT";

// Replaces every occurrence of `key`. The registry's guide is substituted the
// same way the topic and passage are, so a basis added to bases.pbtxt reaches
// the model without this file changing.
void substitute(std::string& prompt, const std::string& key, const std::string& value) {
    size_t at = 0;
    while ((at = prompt.find(key, at)) != std::string::npos) {
        prompt.replace(at, key.size(), value);
        at += value.size();
    }
}

} // namespace

std::string DebateGenerator::BuildPrompt(const std::string& topic, const std::string& passage) {
    std::string truncated = passage;
    if (truncated.size() > kMaxPassageChars) {
        truncated = truncated.substr(0, kMaxPassageChars) + "... [truncated]";
    }

    std::string prompt = kGeneratePromptTemplate;
    substitute(prompt, "{topic}",
               topic.empty() ? "(none given -- take it from the passage)" : topic);
    substitute(prompt, "{bases}", bases::GuideAsList());
    substitute(prompt, "{default_basis}", bases::DefaultName());
    // Last, so a passage that happens to contain a placeholder is not itself
    // substituted into.
    substitute(prompt, "{passage}", truncated);
    return prompt;
}
