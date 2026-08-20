#include "AiChallenger.h"

#include "../debateModerator/DebateModerator.h"
#include "DebateWrapper.h"
#include "Log.h"
#include "debate_event.pb.h"

#include <jsoncons/json.hpp>
#include <set>
#include <string>

namespace {

// Asks for objections a reader could check, not for disagreement. A model told
// simply to "challenge this" produces contrarianism: restating the claim in
// the negative, or objecting that more research is needed -- true of
// everything, and it moves nothing.
const char* kChallengePromptTemplate =
    R"PROMPT(You are reviewing a structured debate and looking for real weaknesses in it.

The debate is a graph. Claims are supported and challenged by other claims. Each claim is shown with an id like [#42].

HOW TO READ IT

- Indentation means the indented block is offered FOR or AGAINST the line above it.
- "SUPPORTED BY" means those claims are the reasons for the claim above.
- "CHALLENGED BY" means those claims already attack it.
- "all of the following TOGETHER" means those claims are ONE argument needing every part. Such an argument is only as strong as its weakest part: if one part is doubtful, the whole line goes.
- "accepted" / "refuted" / "open" is where a claim currently stands.

WHAT MAKES A GOOD CHALLENGE

1. It must be checkable. "The figure counts commits rather than shipped work" can be settled by looking. "This needs more research" cannot, and is true of everything.
2. Attack the weakest real link, not the conclusion you happen to disagree with. A joint argument's weakest part is often the best target in the debate.
3. Say what is actually wrong: the sample does not support the claim, the number measures something else, a term shifts meaning between two claims, the conclusion needs an assumption nobody stated, the source is not in a position to know.
4. Do NOT simply negate the claim. "Remote work does not raise productivity" is a contradiction, not a challenge.
5. Do NOT invent facts, studies or figures. You may say a claim ASSUMES something, or that the stated evidence does not reach as far as the claim does. You may not supply evidence of your own.
6. Do not repeat a challenge the debate already makes. Read the existing ones first.
7. If a claim is well supported and you have no real objection, leave it alone. Three sharp challenges beat eight weak ones.

OUTPUT

Return ONLY JSON, no markdown fence:

{
  "challenges": [
    {
      "target_claim_id": 42,
      "text": "the challenge, as a single claim that could itself be true or false",
      "description": "one sentence on what exactly is wrong, or empty",
      "basis": "EVIDENTIAL"
    }
  ]
}

target_claim_id must be a number that appears as [#id] in the debate below. basis is one of EVIDENTIAL (the evidence does not show this), AUTHORITY (the source is not in a position to know), DEFINITIONAL (the term is used in two senses), NORMATIVE (a value judgement presented as fact), or "" if none fits.

Return between 1 and 5 challenges. If there is genuinely nothing worth challenging, return {"challenges": []}.

THE DEBATE:

{debate})PROMPT";

} // namespace

std::string AiChallenger::BuildPrompt(const std::string& debateText) {
    std::string prompt = kChallengePromptTemplate;
    const std::string key = "{debate}";
    const size_t at = prompt.find(key);
    if (at != std::string::npos) {
        prompt.replace(at, key.size(), debateText);
    }
    return prompt;
}

AiChallenger::Result AiChallenger::Apply(const std::string& json, int user_id,
                                         DebateModerator& moderator) {
    Result result;
    DebateWrapper& debateWrapper = moderator.getDebateWrapper();

    jsoncons::json doc;
    try {
        doc = jsoncons::json::parse(json);
    } catch (const std::exception& e) {
        result.error = std::string("The AI's challenges were not valid JSON: ") + e.what();
        return result;
    }
    if (!doc.contains("challenges") || !doc["challenges"].is_array()) {
        result.error = "The AI returned no challenges list.";
        return result;
    }

    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();
    if (debateId <= 0) {
        result.error = "No debate is open.";
        return result;
    }

    // Which claims actually exist. A challenge naming anything else is dropped
    // rather than placed somewhere plausible: attaching an objection to the
    // wrong sentence puts words into someone else's argument.
    std::set<int> realClaims;
    for (const auto& blob : debateWrapper.getStatementsForDebate(debateId)) {
        debate::Claim claim;
        if (claim.ParseFromArray(blob.data(), static_cast<int>(blob.size()))) {
            realClaims.insert(claim.id());
        }
    }

    auto forge = [user_id](debate_event::EventType type) {
        debate_event::DebateEvent event;
        event.mutable_user()->set_user_id(user_id);
        event.mutable_user()->set_is_logged_in(true);
        event.set_type(type);
        return event;
    };

    for (const auto& entry : doc["challenges"].array_range()) {
        const int targetId =
            entry.contains("target_claim_id") && entry["target_claim_id"].is_number()
                ? entry["target_claim_id"].as<int>()
                : 0;
        const std::string text =
            entry.contains("text") && entry["text"].is_string() ? entry["text"].as<std::string>() : "";
        if (text.empty() || realClaims.find(targetId) == realClaims.end()) {
            result.skipped++;
            continue;
        }

        const std::string description =
            entry.contains("description") && entry["description"].is_string()
                ? entry["description"].as<std::string>()
                : "";
        const std::string basis =
            entry.contains("basis") && entry["basis"].is_string() ? entry["basis"].as<std::string>() : "";

        auto goTo = forge(debate_event::GO_TO_CLAIM);
        goTo.mutable_go_to_claim()->set_claim_id(targetId);
        moderator.handleRequest(goTo);

        auto start = forge(debate_event::START_CHALLENGE_CLAIM);
        moderator.handleRequest(start);

        auto mark = forge(debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
        mark.mutable_add_claim_to_be_challenged()->set_claim_id(targetId);
        moderator.handleRequest(mark);

        auto submit = forge(debate_event::SUBMIT_CHALLENGE_CLAIM);
        auto* payload = submit.mutable_submit_challenge_claim();
        payload->set_challenge_sentence(text);
        payload->set_challenge_description(description);
        payload->set_basis(basis);
        moderator.handleRequest(submit);

        result.challengesAdded++;
    }

    Log::info("[AiChallenger] Added " + std::to_string(result.challengesAdded) +
              " challenge(s), skipped " + std::to_string(result.skipped));
    result.ok = true;
    return result;
}
