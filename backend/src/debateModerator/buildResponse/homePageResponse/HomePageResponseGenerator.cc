#include "HomePageResponseGenerator.h"
#include "../../../utils/pathUtils.h"
#include "debate.pb.h"
#include "user_engagement.pb.h"
#include <iostream>
#include <vector>
#include <map>
#include "../../../utils/Log.h"
#include "../../../utils/DebateBrief.h"
#include "../debatePageResponse/BuildCollection.h"
#include <chrono>

void HomePageResponseGenerator::BuildHomePageResponse(
    moderator_to_vr::ModeratorToVRMessage& responseMessage,
    const int& user_id,
    DebateWrapper& debateWrapper,
    std::map<int, moderator_to_vr::DebateTopic>* cardCache
) {
    Log::debug("[HomePageResponseGenerator] Building Home Page response for user: "
              + std::to_string(user_id));
    const auto started = std::chrono::steady_clock::now();

    // The shelf lists every debate, for everyone: membership is no longer
    // what you see, only who has taken part (a signed-in user becomes a
    // member with their first change in a debate).
    std::vector<int> debateIds = debateWrapper.getAllDebateIds();

    moderator_to_vr::DebateList debateListProto;
    int built = 0;

    // Names are looked up once per page, not once per claim per debate.
    std::map<int, std::string> usernames;
    auto nameOf = [&](int id) -> std::string {
        if (id <= 0) return std::string();
        auto it = usernames.find(id);
        if (it != usernames.end()) return it->second;
        const std::string name = debateWrapper.getUserProtobuf(id).username();
        usernames[id] = name;
        return name;
    };

    for (const int& debateId : debateIds) {
        if (cardCache != nullptr) {
            auto cached = cardCache->find(debateId);
            if (cached != cardCache->end()) {
                *debateListProto.add_topics() = cached->second;
                continue;
            }
        }
        ++built;
        std::vector<uint8_t> debateBytes = debateWrapper.getDebateProtobuf(debateId);
        debate::Debate debateProto;
        if (debateBytes.empty() ||
            !debateProto.ParseFromArray(debateBytes.data(), static_cast<int>(debateBytes.size()))) {
            continue;
        }
        moderator_to_vr::DebateTopic* topicProto = debateListProto.add_topics();
        topicProto->set_id(debateProto.id());
        topicProto->set_topic(debateProto.topic());
        topicProto->set_creator_id(debateProto.creator_id());
        topicProto->set_is_challenge(false);
        topicProto->set_claim_its_challenging("");

        // The card: the same structure the Brief tab is built from, over the
        // same collection the debate page shows.
        const debate::Collection collection = BuildCollection::BuildForDebateAndUsers(
            debateId, debateWrapper.findUsersInDebate(debateId), debateWrapper);
        const DebateBrief::Facts facts = DebateBrief::Compute(collection, nameOf);
        topicProto->set_root_text(facts.rootText);
        topicProto->set_root_holder(facts.rootHolder);
        topicProto->set_claim_count(collection.claims_by_id_size());
        topicProto->set_stance_count(static_cast<int>(facts.stances.size()));
        int openChallenges = 0;
        for (const auto& owed : facts.openByHolder) {
            openChallenges += static_cast<int>(owed.second.size());
        }
        topicProto->set_open_challenge_count(openChallenges);
        for (const DebateBrief::Site& site : facts.sites) {
            if (site.hinge) {
                topicProto->set_unresolved(true);
                topicProto->set_hinge_text(site.moves.empty() ? std::string() : site.moves.front().text);
                break;
            }
        }
        for (const DebateBrief::Stance& stance : facts.stances) {
            topicProto->add_involved(stance.who);
        }
        if (cardCache != nullptr) {
            (*cardCache)[debateId] = *topicProto;
        }
    }

    *responseMessage.mutable_debate_list() = debateListProto;

    const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count();
    Log::info("[HomePageResponseGenerator] Shelf of " + std::to_string(debateListProto.topics_size()) +
              " debates in " + std::to_string(ms) + " ms (" + std::to_string(built) +
              " computed, the rest cached)");
}
