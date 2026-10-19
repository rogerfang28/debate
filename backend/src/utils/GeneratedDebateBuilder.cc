#include "GeneratedDebateBuilder.h"

#include "Log.h"
#include "debate_event.pb.h"

#include <jsoncons/json.hpp>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

struct PlannedArgument {
    std::string target;
    bool challenges = false;
    std::string basis;
    std::vector<std::string> texts;
    std::vector<std::string> descriptions;
    std::vector<std::string> ids;
};

std::string readString(const jsoncons::json& node, const char* key) {
    if (!node.contains(key)) {
        return "";
    }
    const auto& value = node[key];
    return value.is_string() ? value.as<std::string>() : "";
}

// Claim ids currently in a debate. The handlers do not report what they
// create, so each step works out its own additions by difference.
std::set<int> claimIdsIn(int debateId, DebateWrapper& debateWrapper) {
    std::set<int> ids;
    for (const auto& blob : debateWrapper.getStatementsForDebate(debateId)) {
        debate::Claim claim;
        if (claim.ParseFromArray(blob.data(), static_cast<int>(blob.size()))) {
            ids.insert(claim.id());
        }
    }
    return ids;
}

// One event, dispatched exactly as a click from the UI would be.
debate_event::DebateEvent forge(int userId, debate_event::EventType type) {
    debate_event::DebateEvent event;
    event.mutable_user()->set_user_id(userId);
    event.mutable_user()->set_is_logged_in(true);
    event.set_type(type);
    return event;
}

} // namespace

GeneratedDebateBuilder::Result GeneratedDebateBuilder::Build(const std::string& json, int user_id,
                                                             DebateModerator& moderator) {
    Result result;
    DebateWrapper& debateWrapper = moderator.getDebateWrapper();

    jsoncons::json doc;
    try {
        doc = jsoncons::json::parse(json);
    } catch (const std::exception& e) {
        result.error = std::string("The generated debate was not valid JSON: ") + e.what();
        return result;
    }

    const std::string rootText = doc.contains("root") ? readString(doc["root"], "text") : "";
    if (rootText.empty()) {
        result.error = "The generated debate has no root claim.";
        return result;
    }
    if (!doc.contains("arguments") || !doc["arguments"].is_array() || doc["arguments"].empty()) {
        result.error = "The generated debate has no arguments.";
        return result;
    }

    // Plan and validate EVERYTHING before creating anything. A debate half
    // built from bad JSON leaves the user to find and delete it.
    std::vector<PlannedArgument> plan;
    std::set<std::string> knownIds{"root"};

    for (const auto& argument : doc["arguments"].array_range()) {
        const std::string target = readString(argument, "target");
        if (target.empty() || knownIds.find(target) == knownIds.end()) {
            result.error = "An argument targets '" + target +
                           "', which is not the root or a claim introduced before it.";
            return result;
        }
        if (!argument.contains("claims") || !argument["claims"].is_array() ||
            argument["claims"].empty()) {
            result.error = "An argument introduces no claims.";
            return result;
        }

        PlannedArgument planned;
        planned.target = target;
        planned.challenges = readString(argument, "kind") == "challenges";
        planned.basis = readString(argument, "basis");

        for (const auto& claim : argument["claims"].array_range()) {
            const std::string text = readString(claim, "text");
            const std::string id = readString(claim, "id");
            if (text.empty() || id.empty()) {
                result.error = "A generated claim has no text or no id.";
                return result;
            }
            if (!knownIds.insert(id).second) {
                result.error = "Claim id '" + id + "' is used twice.";
                return result;
            }
            planned.texts.push_back(text);
            planned.descriptions.push_back(readString(claim, "description"));
            planned.ids.push_back(id);
        }

        // GROUPING IS SPLIT UNLESS JUSTIFIED.
        //
        // Grouping declares claims JOINT -- the argument dies if any one of
        // them is wrong -- and the model groups by default. On one passage it
        // returned four independent metrics as a single joint argument, then
        // merged two unrelated challenges, and never wrote the joint_reason the
        // prompt demands. Three prompt revisions did not move it.
        //
        // Enforced here instead. With no stated reason the claims become
        // independent arguments: the conservative reading, and the opposite of
        // the direction the model errs in.
        const std::string jointReason = readString(argument, "joint_reason");
        if (planned.texts.size() > 1 && jointReason.empty()) {
            result.groupsSplit++;
            for (size_t i = 0; i < planned.texts.size(); ++i) {
                PlannedArgument single;
                single.target = planned.target;
                single.challenges = planned.challenges;
                single.basis = planned.basis;
                single.texts.push_back(planned.texts[i]);
                single.descriptions.push_back(planned.descriptions[i]);
                single.ids.push_back(planned.ids[i]);
                plan.push_back(single);
            }
            continue;
        }
        plan.push_back(planned);
    }

    const std::vector<int> debatesBefore = debateWrapper.getUserDebateIds(user_id);
    {
        auto event = forge(user_id, debate_event::CREATE_DEBATE);
        event.mutable_create_debate()->set_debate_topic(rootText);
        moderator.handleRequest(event);
    }

    int debateId = 0;
    for (int id : debateWrapper.getUserDebateIds(user_id)) {
        if (std::find(debatesBefore.begin(), debatesBefore.end(), id) == debatesBefore.end()) {
            debateId = id;
        }
    }
    if (debateId <= 0) {
        result.error = "The debate could not be created.";
        return result;
    }
    result.debateId = debateId;

    // AddDebate creates the debate but does not put the author inside it, and
    // the write handlers all act on where the user IS. Without this the claims
    // are attempted against debate 0 and silently nothing is created.
    {
        auto event = forge(user_id, debate_event::ENTER_DEBATE);
        event.mutable_enter_debate()->set_debate_id(debateId);
        moderator.handleRequest(event);
    }

    std::set<int> existing = claimIdsIn(debateId, debateWrapper);
    if (existing.size() != 1) {
        result.error = "The new debate did not come back with exactly one root claim.";
        return result;
    }
    std::map<std::string, int> claimIdByName{{"root", *existing.begin()}};
    result.claimsCreated = 1;

    for (const PlannedArgument& argument : plan) {
        const auto targetIt = claimIdByName.find(argument.target);
        if (targetIt == claimIdByName.end()) {
            result.error = "Internal error: target '" + argument.target + "' was never created.";
            return result;
        }

        // The handlers act on where the user IS, so navigating is part of the
        // write -- exactly as it is when a person clicks through the UI.
        {
            auto event = forge(user_id, debate_event::GO_TO_CLAIM);
            event.mutable_go_to_claim()->set_claim_id(targetIt->second);
            moderator.handleRequest(event);
        }

        if (argument.challenges) {
            auto start = forge(user_id, debate_event::START_CHALLENGE_CLAIM);
            moderator.handleRequest(start);

            auto mark = forge(user_id, debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
            mark.mutable_add_claim_to_be_challenged()->set_claim_id(targetIt->second);
            moderator.handleRequest(mark);

            auto submit = forge(user_id, debate_event::SUBMIT_CHALLENGE_CLAIM);
            auto* payload = submit.mutable_submit_challenge_claim();
            payload->set_challenge_sentence(argument.texts.front());
            payload->set_challenge_description(argument.descriptions.front());
            payload->set_basis(argument.basis);
            for (size_t i = 1; i < argument.texts.size(); ++i) {
                payload->add_extra_sentences(argument.texts[i]);
                payload->add_extra_descriptions(argument.descriptions[i]);
            }
            moderator.handleRequest(submit);
        } else {
            auto event = forge(user_id, debate_event::ADD_CHILD_CLAIM);
            auto* payload = event.mutable_add_child_claim();
            payload->set_claim(argument.texts.front());
            payload->set_description(argument.descriptions.front());
            payload->set_basis(argument.basis);
            for (size_t i = 1; i < argument.texts.size(); ++i) {
                payload->add_extra_claims(argument.texts[i]);
                payload->add_extra_descriptions(argument.descriptions[i]);
            }
            moderator.handleRequest(event);
        }

        // Match the claims this call created back to their generated ids, in
        // creation order, so later arguments can target them.
        const std::set<int> after = claimIdsIn(debateId, debateWrapper);
        std::vector<int> created;
        for (int id : after) {
            if (existing.find(id) == existing.end()) {
                created.push_back(id);
            }
        }
        std::sort(created.begin(), created.end());
        if (created.size() != argument.texts.size()) {
            result.error = "Expected " + std::to_string(argument.texts.size()) +
                           " new claims but the debate gained " + std::to_string(created.size()) + ".";
            return result;
        }
        for (size_t i = 0; i < created.size(); ++i) {
            claimIdByName[argument.ids[i]] = created[i];
        }
        existing = after;
        result.claimsCreated += static_cast<int>(created.size());
        result.argumentsCreated++;
    }

    Log::info("[GeneratedDebateBuilder] Built debate " + std::to_string(debateId) + " with " +
              std::to_string(result.claimsCreated) + " claims across " +
              std::to_string(result.argumentsCreated) + " arguments (" +
              std::to_string(result.groupsSplit) + " unjustified group(s) split)");
    result.ok = true;
    return result;
}
