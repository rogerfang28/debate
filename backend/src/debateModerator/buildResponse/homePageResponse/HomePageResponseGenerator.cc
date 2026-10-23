#include "HomePageResponseGenerator.h"
#include "../../../utils/pathUtils.h"
#include "debate.pb.h"
#include "user_engagement.pb.h"
#include <iostream>
#include <vector>
#include <map>
#include "../../../utils/Log.h"
#include "../../../utils/DebateBrief.h"
#include "../../../utils/Ordered.h"
#include "../../../utils/ClaimVoice.h"
#include "../../../utils/AnonymousUser.h"
#include "../../../utils/UsernamePolicy.h"
#include "user.pb.h"
#include "../debatePageResponse/BuildCollection.h"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <functional>
#include <set>

namespace {

std::string lower(const std::string& s) {
    std::string out(s);
    std::transform(out.begin(), out.end(), out.begin(),
                   [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
    return out;
}

// The number after `"key":` in a move's JSON payload, or 0.
int payloadInt(const std::string& payload, const std::string& key) {
    const std::string needle = "\"" + key + "\":";
    const size_t at = payload.find(needle);
    return at == std::string::npos ? 0 : std::atoi(payload.c_str() + at + needle.size());
}

// The move log, read for the home page: when the debate last moved, and its
// newest few moves in words. Who made a move is the claim's voice where
// there is one (an imported case is entered by "guest" in many voices), else
// the account that made it. A challenge is logged as ASSERT (the challenge
// claim) then OPPOSE; only the OPPOSE is shown. An OPPOSE against a claim
// that is itself a challenge is a reply.
void fillFromMoveLog(moderator_to_vr::DebateTopic* topic, const std::vector<debate::Move>& moves,
                     const DebateBrief::Facts& facts,
                     const std::function<std::string(int)>& nameOf) {
    std::map<int, std::string> whoByClaim;
    std::set<int> challengeClaims;
    for (const DebateBrief::Move& m : facts.timeline) {
        whoByClaim[m.claimId] = m.who;
        if (m.challenge) challengeClaims.insert(m.claimId);
    }
    long long last = 0;
    for (const debate::Move& m : moves) {
        last = std::max<long long>(last, m.created_at().seconds());
    }
    topic->set_last_moved_at(last);

    // An agreement taken back later is not news: (actor, claim) pairs whose
    // newer move was an UNACCEPT.
    std::set<std::pair<int, int>> withdrawn;
    for (auto it = moves.rbegin(); it != moves.rend() && topic->recent_moves_size() < 4; ++it) {
        const debate::Move& m = *it;
        const bool onClaim = m.target_type() == debate::TARGET_CLAIM;
        if (m.type() == debate::UNACCEPT) {
            withdrawn.insert({m.actor_id(), m.target_id()});
            continue;
        }
        if (m.type() == debate::ACCEPT && withdrawn.count({m.actor_id(), m.target_id()})) continue;
        std::string verb;
        std::string who;
        switch (m.type()) {
            case debate::ASSERT:
                if (!onClaim || challengeClaims.count(m.target_id())) continue;
                verb = "claimed";
                who = whoByClaim[m.target_id()];
                break;
            case debate::OPPOSE: {
                verb = onClaim && challengeClaims.count(m.target_id()) ? "replied" : "challenged";
                who = whoByClaim[payloadInt(m.payload(), "challenging_claim_id")];
                break;
            }
            case debate::ACCEPT:
                verb = "agreed";
                break;
            case debate::CONCEDE:
                verb = "conceded";
                break;
            default:
                continue;
        }
        if (who.empty()) {
            const int voice = payloadInt(m.payload(), "voice_id");
            who = nameOf(voice > 0 ? voice : m.actor_id());
        }
        if (who.empty()) continue;
        moderator_to_vr::RecentMove* recent = topic->add_recent_moves();
        recent->set_who(who);
        recent->set_verb(verb);
        recent->set_at(m.created_at().seconds());
        recent->set_seq(m.seq());
    }
}

// Each person's part in this debate, for their profile. Claims and
// challenges go to whose claim it is (the voice, else the entrant), so a
// voice is credited with what was entered in their name; agreements (those
// still standing) and concessions are moves the person made. Anyone who made
// any move gets an entry, so the debate is listed on their profile.
void fillTallies(moderator_to_vr::DebateTopic* topic, const debate::Collection& collection,
                 const DebateBrief::Facts& facts, const std::vector<debate::Move>& moves) {
    std::map<int, moderator_to_vr::PersonTally> by;
    auto of = [&](int id) -> moderator_to_vr::PersonTally& {
        moderator_to_vr::PersonTally& t = by[id];
        t.set_user_id(id);
        return t;
    };
    std::set<int> challengeClaims;
    for (const DebateBrief::Move& m : facts.timeline) {
        if (m.challenge) challengeClaims.insert(m.claimId);
    }
    for (const auto& entry : collection.claims_by_id()) {
        const debate::Claim& claim = entry.second;
        const int holder = ClaimVoiceId(claim);
        if (holder <= 0) continue;
        moderator_to_vr::PersonTally& t = of(holder);
        if (challengeClaims.count(claim.id())) {
            t.set_challenges(t.challenges() + 1);
        } else {
            t.set_claims(t.claims() + 1);
        }
        if (claim.voice_id() > 0 && claim.voice_id() != claim.creator_id()) {
            t.set_voiced(t.voiced() + 1);
        }
    }
    std::set<std::pair<int, int>> agreed;  // (actor, claim) still standing
    for (const debate::Move& m : moves) {
        if (m.actor_id() <= 0) continue;
        moderator_to_vr::PersonTally& t = of(m.actor_id());
        switch (m.type()) {
            case debate::ACCEPT:
                agreed.insert({m.actor_id(), m.target_id()});
                break;
            case debate::UNACCEPT:
                agreed.erase({m.actor_id(), m.target_id()});
                break;
            case debate::CONCEDE:
                t.set_concessions(t.concessions() + 1);
                break;
            default:
                break;
        }
    }
    for (const auto& pair : agreed) {
        moderator_to_vr::PersonTally& t = of(pair.first);
        t.set_agreements(t.agreements() + 1);
    }
    for (const auto& entry : by) {
        *topic->add_tallies() = entry.second;
    }
}

// The profile open on the viewer's home page, summed over every card.
class ProfileBuilder {
public:
    ProfileBuilder(int viewerId, int targetId, DebateWrapper& debateWrapper) : target_(targetId) {
        if (target_ <= 0) return;
        account_ = debateWrapper.getUserProtobuf(target_);
        profile_.set_user_id(target_);
        profile_.set_username(account_.username());
        profile_.set_joined_at(account_.joined_at());
        profile_.set_own(viewerId == target_);
    }

    // One debate's card, before its tallies are cleared.
    void add(const moderator_to_vr::DebateTopic& topic) {
        if (target_ <= 0) return;
        bool part = false;
        for (const auto& t : topic.tallies()) {
            if (t.user_id() != target_) continue;
            part = true;
            profile_.set_claims(profile_.claims() + t.claims());
            profile_.set_challenges(profile_.challenges() + t.challenges());
            profile_.set_agreements(profile_.agreements() + t.agreements());
            profile_.set_concessions(profile_.concessions() + t.concessions());
            voiced_ += t.voiced();
        }
        for (const int member : topic.member_ids()) {
            if (member == target_) part = true;
        }
        const std::string& name = account_.username();
        for (const auto& owed : topic.owed()) {
            if (!name.empty() && owed.who() == name) {
                part = true;
                profile_.set_open_challenges(profile_.open_challenges() + 1);
            }
        }
        if (part) {
            profile_.add_debate_ids(topic.id());
            profile_.set_debates(profile_.debates() + 1);
        }
    }

    void finish(moderator_to_vr::ModeratorToVRMessage& res) {
        if (target_ <= 0) return;
        const std::string& name = account_.username();
        std::string kind = "member";
        if (name.empty()) {
            kind = "missing";
        } else if (anonymous_user::IsAnonymousUsername(name)) {
            kind = "visitor";
        } else if (username_policy::IsDeletedName(name)) {
            kind = "deleted";
        } else if (account_.password_hash().empty() && account_.google_sub().empty() && voiced_ > 0) {
            // Never signs in; claims are entered in their name from sources.
            kind = "voice";
        }
        profile_.set_kind(kind);
        if (kind != "member" && kind != "voice") {
            // Nothing about a visitor or a closed account is shown.
            moderator_to_vr::Profile bare;
            bare.set_user_id(target_);
            bare.set_kind(kind);
            profile_ = bare;
        }
        *res.mutable_profile() = profile_;
    }

private:
    int target_ = 0;
    int voiced_ = 0;
    user::User account_;
    moderator_to_vr::Profile profile_;
};

}  // namespace

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

    // Explore's search, case-insensitive, over each debate's title and every
    // claim sentence (kept on the cached card for this).
    const std::string query = lower(responseMessage.user().engagement().home_view().explore_query());
    auto search = [&](moderator_to_vr::DebateTopic* topic) {
        topic->clear_matched_claims();
        topic->set_search_match(false);
        if (query.empty()) return;
        bool match = lower(topic->topic()).find(query) != std::string::npos ||
                     lower(topic->root_text()).find(query) != std::string::npos;
        for (const std::string& claim : topic->claim_texts()) {
            if (lower(claim).find(query) == std::string::npos) continue;
            match = true;
            if (topic->matched_claims_size() < 2) topic->add_matched_claims(claim);
            else break;
        }
        topic->set_search_match(match);
    };

    moderator_to_vr::DebateList debateListProto;
    int built = 0;
    ProfileBuilder profile(user_id, responseMessage.user().engagement().home_view().profile_user_id(),
                           debateWrapper);

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
                moderator_to_vr::DebateTopic* topic = debateListProto.add_topics();
                *topic = cached->second;
                search(topic);
                topic->clear_claim_texts();
                profile.add(*topic);
                topic->clear_tallies();
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
        const std::vector<int> memberIds = debateWrapper.findUsersInDebate(debateId);
        const debate::Collection collection = BuildCollection::BuildForDebateAndUsers(
            debateId, memberIds, debateWrapper);
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

        // Everyone with a claim or a stance.
        std::set<std::string> people;
        for (const DebateBrief::Stance& stance : facts.stances) people.insert(stance.who);
        for (const DebateBrief::Move& m : facts.timeline) people.insert(m.who);
        people.erase(std::string());
        topicProto->set_people_count(static_cast<int>(people.size()));

        // Who owes replies, most first; and each open challenge with its text.
        // People by name (case-insensitive), as on the debate page.
        std::vector<std::pair<std::string, int>> owing;
        for (const std::string& who : ordered::SortedNames(facts.openByHolder)) {
            const std::vector<int>& list = facts.openByHolder.at(who);
            if (list.empty()) continue;
            owing.emplace_back(who, static_cast<int>(list.size()));
            for (const int claimId : list) {
                moderator_to_vr::OwedReply* reply = topicProto->add_owed();
                reply->set_who(who);
                reply->set_claim_id(claimId);
                auto claim = collection.claims_by_id().find(claimId);
                if (claim != collection.claims_by_id().end()) reply->set_text(claim->second.sentence());
            }
        }
        std::stable_sort(owing.begin(), owing.end(),
                         [](const auto& a, const auto& b) { return a.second > b.second; });
        for (const auto& party : owing) {
            moderator_to_vr::OwingParty* out = topicProto->add_owing();
            out->set_who(party.first);
            out->set_count(party.second);
        }

        for (const int member : memberIds) topicProto->add_member_ids(member);
        // In the order they were made, so a search shows the earliest matches.
        for (const int claimId : ordered::SortedClaimIds(collection)) {
            topicProto->add_claim_texts(collection.claims_by_id().at(claimId).sentence());
        }
        const std::vector<debate::Move> moves = debateWrapper.getMovesForDebate(debateId);
        fillFromMoveLog(topicProto, moves, facts, nameOf);
        fillTallies(topicProto, collection, facts, moves);

        if (cardCache != nullptr) {
            (*cardCache)[debateId] = *topicProto;
        }
        search(topicProto);
        topicProto->clear_claim_texts();
        profile.add(*topicProto);
        topicProto->clear_tallies();
    }

    *responseMessage.mutable_debate_list() = debateListProto;
    profile.finish(responseMessage);

    const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started).count();
    Log::info("[HomePageResponseGenerator] Shelf of " + std::to_string(debateListProto.topics_size()) +
              " debates in " + std::to_string(ms) + " ms (" + std::to_string(built) +
              " computed, the rest cached)");
}
