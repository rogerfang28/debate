#include "DebateBrief.h"

#include "ClaimVoice.h"

#include <jsoncons/json.hpp>

#include <algorithm>
#include <functional>
#include <iomanip>
#include <queue>
#include <set>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

namespace DebateBrief {

namespace {

const debate::Claim* claimOf(const debate::Collection& c, int id) {
    auto it = c.claims_by_id().find(id);
    return it == c.claims_by_id().end() ? nullptr : &it->second;
}

// The claim a relation is ultimately aimed at. A challenge to an argument
// (to_relation_id) lands on the claim that argument is for.
int targetClaimOf(const debate::Collection& c, const debate::Relationship& rel, bool* viaRelation) {
    if (viaRelation) *viaRelation = false;
    const debate::Relationship* cur = &rel;
    for (int guard = 0; guard < 8; ++guard) {
        if (cur->to_case() == debate::Relationship::kToClaimId) {
            return cur->to_claim_id();
        }
        if (cur->to_case() != debate::Relationship::kToRelationId) {
            return 0;
        }
        if (viaRelation) *viaRelation = true;
        auto it = c.links_by_id().find(cur->to_relation_id());
        if (it == c.links_by_id().end()) return 0;
        cur = &it->second;
    }
    return 0;
}

std::string joinNames(const std::set<std::string>& names) {
    std::string out;
    for (const std::string& n : names) {
        if (!out.empty()) out += ", ";
        out += n;
    }
    return out;
}

std::string joinSentences(const debate::Collection& c, const debate::Relationship& rel) {
    std::string out;
    for (int from : rel.from_claim_ids()) {
        const debate::Claim* cl = claimOf(c, from);
        if (!cl) continue;
        if (!out.empty()) out += "  +  ";
        out += cl->sentence();
    }
    return out;
}

}  // namespace

Facts Compute(const debate::Collection& c, const NameOf& nameOf) {
    Facts facts;

    auto holderOf = [&](int claimId) -> std::string {
        const debate::Claim* cl = claimOf(c, claimId);
        return cl ? nameOf(ClaimVoiceId(*cl)) : std::string();
    };
    // Whether `who` holds a claim true: an explicit status, else it is theirs.
    auto holdsTrue = [&](const debate::Claim& cl, const std::string& who) {
        auto it = cl.user_statuses().find(who);
        if (it != cl.user_statuses().end()) return it->second == debate::TRUE_CLAIM;
        return nameOf(ClaimVoiceId(cl)) == who;
    };
    auto statusFor = [&](const debate::Claim& cl, const std::string& who) {
        auto it = cl.user_statuses().find(who);
        if (it != cl.user_statuses().end()) return it->second;
        return nameOf(ClaimVoiceId(cl)) == who ? debate::TRUE_CLAIM : debate::UNDETERMINED;
    };

    // Relations, sorted by id so every list below comes out in the order the
    // moves were made.
    std::vector<const debate::Relationship*> supports;
    std::vector<const debate::Relationship*> challenges;
    for (const auto& entry : c.links_by_id()) {
        (entry.second.role() == debate::OPPOSES ? challenges : supports).push_back(&entry.second);
    }
    auto byId = [](const debate::Relationship* a, const debate::Relationship* b) { return a->id() < b->id(); };
    std::sort(supports.begin(), supports.end(), byId);
    std::sort(challenges.begin(), challenges.end(), byId);

    // Which claims are challenges, and who has challenged each claim back.
    std::unordered_set<int> challengeClaims;
    std::unordered_map<int, std::set<std::string>> counteredBy;
    std::unordered_map<int, std::vector<const debate::Relationship*>> challengesOn;  // target -> relations
    for (const debate::Relationship* rel : challenges) {
        const int target = targetClaimOf(c, *rel, nullptr);
        for (int from : rel->from_claim_ids()) {
            challengeClaims.insert(from);
            const std::string by = holderOf(from);
            if (!by.empty() && target > 0) counteredBy[target].insert(by);
        }
        if (target > 0) challengesOn[target].push_back(rel);
    }

    // Parents and children along the support relations.
    std::unordered_map<int, int> parentOf;
    std::unordered_map<int, std::vector<int>> childrenOf;
    for (const debate::Relationship* rel : supports) {
        const int target = targetClaimOf(c, *rel, nullptr);
        if (target <= 0) continue;
        for (int from : rel->from_claim_ids()) {
            if (!parentOf.count(from)) parentOf[from] = target;
            childrenOf[target].push_back(from);
        }
    }

    // The main claim: the lowest-numbered claim nothing is offered for or
    // against anything with.
    std::vector<int> claimIds;
    for (const auto& entry : c.claims_by_id()) claimIds.push_back(entry.first);
    std::sort(claimIds.begin(), claimIds.end());
    for (int id : claimIds) {
        if (!parentOf.count(id) && !challengeClaims.count(id)) {
            facts.rootClaimId = id;
            break;
        }
    }
    if (facts.rootClaimId == 0 && !claimIds.empty()) facts.rootClaimId = claimIds.front();
    if (const debate::Claim* root = claimOf(c, facts.rootClaimId)) {
        facts.rootText = root->sentence();
        facts.rootHolder = holderOf(facts.rootClaimId);
    }

    // Depth: distance from the main claim, down supports and across
    // challenges alike.
    std::unordered_map<int, int> depth;
    {
        std::queue<int> q;
        depth[facts.rootClaimId] = 0;
        q.push(facts.rootClaimId);
        while (!q.empty()) {
            const int id = q.front();
            q.pop();
            const int d = depth[id];
            for (int child : childrenOf[id]) {
                if (!depth.count(child)) { depth[child] = d + 1; q.push(child); }
            }
            for (const debate::Relationship* rel : challengesOn[id]) {
                for (int from : rel->from_claim_ids()) {
                    if (!depth.count(from)) { depth[from] = d + 1; q.push(from); }
                }
            }
        }
    }
    auto depthOf = [&](int id) { auto it = depth.find(id); return it == depth.end() ? 99 : it->second; };

    // Is a challenge (relation `rel`, its first claim `challengeId`, aimed at
    // `target`) still waiting on `holder`? The Debate tab's rule: answered
    // when the holder challenged it back, sees it as false, or no longer
    // holds the claim it attacks.
    enum class Answer { NotTheirs, Open, Countered, SeesFalse, Conceded, Withdrawn };
    auto answerOf = [&](int challengeId, int target, const std::string& holder) {
        const debate::Claim* targetClaim = claimOf(c, target);
        const debate::Claim* challengeClaim = claimOf(c, challengeId);
        if (!targetClaim || !challengeClaim) return Answer::NotTheirs;
        if (holder.empty() || holder == holderOf(challengeId)) return Answer::NotTheirs;
        auto cb = counteredBy.find(challengeId);
        if (cb != counteredBy.end() && cb->second.count(holder)) return Answer::Countered;
        const debate::ClaimStatus view = statusFor(*challengeClaim, holder);
        if (view == debate::FALSE_CLAIM) return Answer::SeesFalse;
        if (!holdsTrue(*targetClaim, holder)) {
            return view == debate::TRUE_CLAIM ? Answer::Conceded : Answer::Withdrawn;
        }
        return Answer::Open;
    };
    // Everyone who holds a claim true.
    auto holdersOf = [&](int claimId) {
        std::set<std::string> out;
        const debate::Claim* cl = claimOf(c, claimId);
        if (!cl) return out;
        const std::string owner = nameOf(ClaimVoiceId(*cl));
        if (!owner.empty() && holdsTrue(*cl, owner)) out.insert(owner);
        for (const auto& entry : cl->user_statuses()) {
            if (entry.second == debate::TRUE_CLAIM) out.insert(entry.first);
        }
        return out;
    };

    // Open challenges by holder, over every challenge.
    for (const debate::Relationship* rel : challenges) {
        const int target = targetClaimOf(c, *rel, nullptr);
        if (target <= 0 || rel->from_claim_ids_size() == 0) continue;
        const int challengeId = rel->from_claim_ids(0);
        for (const std::string& holder : holdersOf(target)) {
            if (answerOf(challengeId, target, holder) == Answer::Open) {
                facts.openByHolder[holder].push_back(challengeId);
            }
        }
    }

    // Battle sites: one per challenged claim that is not itself a
    // challenge, holding every challenge to it and every reply.
    struct SiteBuild {
        Site site;
        std::set<std::string> owed;
        std::set<std::string> conceded;
    };
    std::vector<SiteBuild> builds;
    std::unordered_map<int, size_t> siteOf;  // target claim -> index in builds
    std::unordered_set<int> placed;
    std::function<void(SiteBuild&, const debate::Relationship*, int)> addChallengeMoves =
        [&](SiteBuild& b, const debate::Relationship* rel, int target) {
            if (rel->from_claim_ids_size() == 0) return;
            const int challengeId = rel->from_claim_ids(0);
            if (placed.count(challengeId)) return;
            placed.insert(challengeId);
            bool viaRelation = false;
            targetClaimOf(c, *rel, &viaRelation);
            Move m;
            m.claimId = challengeId;
            m.who = holderOf(challengeId);
            m.text = joinSentences(c, *rel);
            m.challenge = true;
            m.reply = challengeClaims.count(target) > 0;
            m.againstReasoning = viaRelation;
            b.site.moves.push_back(m);

            // Where this challenge stands with the holders of what it attacks.
            for (const std::string& holder : holdersOf(target)) {
                const Answer a = answerOf(challengeId, target, holder);
                if (a == Answer::Open) b.owed.insert(holder);
                if (a == Answer::Conceded) b.conceded.insert(holder);
            }
            // Also the holders who USED to hold the target and conceded to
            // this very challenge: they no longer hold it, so holdersOf
            // misses them.
            if (const debate::Claim* targetClaim = claimOf(c, target)) {
                for (const auto& entry : targetClaim->user_statuses()) {
                    if (entry.second == debate::FALSE_CLAIM &&
                        answerOf(challengeId, target, entry.first) == Answer::Conceded) {
                        b.conceded.insert(entry.first);
                    }
                }
            }

            // Replies to this challenge, in order, each with its own replies.
            for (const debate::Relationship* reply : challengesOn[challengeId]) {
                addChallengeMoves(b, reply, challengeId);
            }
        };

    for (const debate::Relationship* rel : challenges) {
        const int target = targetClaimOf(c, *rel, nullptr);
        if (target <= 0 || rel->from_claim_ids_size() == 0) continue;
        if (challengeClaims.count(target)) continue;  // a reply; it rides in its site
        if (placed.count(rel->from_claim_ids(0))) continue;
        const debate::Claim* targetClaim = claimOf(c, target);
        if (!targetClaim) continue;

        auto found = siteOf.find(target);
        if (found == siteOf.end()) {
            SiteBuild b;
            b.site.id = target;
            b.site.targetClaimId = target;
            b.site.depth = depthOf(target);
            Move first;
            first.claimId = target;
            first.who = holderOf(target);
            first.text = targetClaim->sentence();
            b.site.moves.push_back(first);
            siteOf[target] = builds.size();
            builds.push_back(b);
            found = siteOf.find(target);
        }
        addChallengeMoves(builds[found->second], rel, target);
    }
    for (SiteBuild& b : builds) {
        if (!b.owed.empty()) {
            b.site.open = true;
            b.site.turn = joinNames(b.owed);
        } else if (!b.conceded.empty()) {
            b.site.outcome = joinNames(b.conceded) + " conceded";
        } else {
            b.site.outcome = "Answered";
        }
        facts.sites.push_back(b.site);
    }

    // Order: hinge, then open by nearness to the root, then the rest.
    std::stable_sort(facts.sites.begin(), facts.sites.end(), [](const Site& a, const Site& b) {
        if (a.open != b.open) return a.open;
        if (a.depth != b.depth) return a.depth < b.depth;
        return a.id < b.id;
    });
    if (!facts.sites.empty() && facts.sites.front().open) facts.sites.front().hinge = true;

    // Stances on the main claim: everyone who holds something true here, or
    // has said where they stand on the main claim.
    if (const debate::Claim* root = claimOf(c, facts.rootClaimId)) {
        std::set<std::string> people;
        for (int id : claimIds) {
            for (const std::string& h : holdersOf(id)) people.insert(h);
        }
        for (const auto& entry : root->user_statuses()) {
            if (entry.second != debate::UNDETERMINED) people.insert(entry.first);
        }
        for (const std::string& who : people) {
            Stance s;
            s.who = who;
            s.status = statusFor(*root, who);
            facts.stances.push_back(s);
        }
        auto rank = [](debate::ClaimStatus st) { return st == debate::TRUE_CLAIM ? 0 : st == debate::FALSE_CLAIM ? 1 : 2; };
        std::stable_sort(facts.stances.begin(), facts.stances.end(), [&](const Stance& a, const Stance& b) {
            return rank(a.status) < rank(b.status);
        });
    }

    // Common ground, the timeline, the sources.
    std::set<std::string> seenUrls;
    for (int id : claimIds) {
        const debate::Claim* cl = claimOf(c, id);
        if (!cl) continue;
        Move m;
        m.claimId = id;
        m.who = holderOf(id);
        m.text = cl->sentence();
        m.challenge = challengeClaims.count(id) > 0;
        facts.timeline.push_back(m);
        if (!challengesOn.count(id) && holdersOf(id).size() >= 2) {
            facts.commonGround.push_back(m);
        }
        for (const debate::Source& src : cl->sources()) {
            if (!src.url().empty() && seenUrls.insert(src.url()).second) {
                facts.sources.push_back(src.url());
            }
        }
    }

    return facts;
}

std::string FactsText(const Facts& facts) {
    std::ostringstream out;
    out << "MAIN CLAIM [#" << facts.rootClaimId << "] by " << facts.rootHolder << ": " << facts.rootText << "\n";
    out << "STANCES ON THE MAIN CLAIM:";
    for (const Stance& s : facts.stances) {
        out << " " << s.who << "=" << (s.status == debate::TRUE_CLAIM ? "true" : s.status == debate::FALSE_CLAIM ? "false" : "undecided") << ";";
    }
    out << "\n";
    out << "BATTLE SITES (id, then the exchange, oldest first):\n";
    for (const Site& site : facts.sites) {
        out << "SITE " << site.id << (site.hinge ? " [HINGE: the open challenge nearest the main claim]" : "") << "\n";
        for (const Move& m : site.moves) {
            out << "  " << (!m.challenge ? "claim, " : m.reply ? "reply, " : m.againstReasoning ? "challenges the reasoning, " : "challenges, ")
                << m.who << " [#" << m.claimId << "]: " << m.text << "\n";
        }
        out << "  status: " << (site.open ? "OPEN, waiting on " + site.turn : site.outcome) << "\n";
    }
    out << "COMMON GROUND (claims nobody challenged, held by more than one person):\n";
    for (const Move& m : facts.commonGround) {
        out << "  [#" << m.claimId << "] " << m.text << "\n";
    }
    return out.str();
}

std::string Fingerprint(const Facts& facts) {
    const std::size_t h = std::hash<std::string>{}(FactsText(facts));
    std::ostringstream out;
    out << std::hex << std::setw(16) << std::setfill('0') << static_cast<unsigned long long>(h);
    return out.str();
}

std::string Prompt(const Facts& facts, const std::string& topic) {
    std::ostringstream out;
    out << "You are writing the front page of a structured debate for someone who has never seen it.\n"
           "The structure below was computed from the debate and is correct. Do not question it, do not add "
           "to it, and do not mention anything that is not in it: no outside facts, no people or events not "
           "named here.\n\n"
           "Return ONLY a JSON object, no prose around it, with exactly these keys:\n"
           "  \"verdict\": one sentence. Say whether the debate is settled or not, and name the one question "
           "it hangs on (the HINGE site), in everyday words. Example: \"Unresolved. It hangs on whether a "
           "pardoned witness can still plead the Fifth, and Paul has not answered that.\" If no site is open, "
           "say that nothing is currently challenged.\n"
           "  \"story\": at most three short sentences telling a newcomer what happened and what is being "
           "argued. Plain words, no jargon, no claim numbers.\n"
           "  \"sites\": an object mapping each SITE id (as a string) to a name for that battle site: a "
           "question of at most eight words that a newcomer would understand, such as \"Does the pardon "
           "cancel the Fifth?\". Every site listed below must appear.\n\n"
           "Do not decide who is right. Do not use markdown.\n\n";
    if (!topic.empty()) out << "TOPIC: " << topic << "\n\n";
    out << FactsText(facts);
    return out.str();
}

namespace {

std::string stripFence(const std::string& reply) {
    std::string s = reply;
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    s = s.substr(a);
    if (s.rfind("```", 0) == 0) {
        const size_t nl = s.find('\n');
        s = nl == std::string::npos ? "" : s.substr(nl + 1);
        const size_t end = s.rfind("```");
        if (end != std::string::npos) s = s.substr(0, end);
    }
    // Some models talk before the JSON; take from the first brace.
    const size_t brace = s.find('{');
    if (brace == std::string::npos) return "";
    const size_t last = s.rfind('}');
    if (last == std::string::npos || last < brace) return "";
    return s.substr(brace, last - brace + 1);
}

}  // namespace

bool ParseProse(const std::string& reply, Prose* prose, std::string* error) {
    const std::string body = stripFence(reply);
    if (body.empty()) {
        if (error) *error = "The model did not return JSON.";
        return false;
    }
    try {
        const jsoncons::json j = jsoncons::json::parse(body);
        if (!j.is_object()) {
            if (error) *error = "The model's reply was not a JSON object.";
            return false;
        }
        Prose out;
        if (j.contains("verdict") && j["verdict"].is_string()) out.verdict = j["verdict"].as<std::string>();
        if (j.contains("story") && j["story"].is_string()) out.story = j["story"].as<std::string>();
        if (j.contains("sites") && j["sites"].is_object()) {
            for (const auto& item : j["sites"].object_range()) {
                try {
                    const int id = std::stoi(std::string(item.key()));
                    if (item.value().is_string()) out.siteNames[id] = item.value().as<std::string>();
                } catch (...) {
                    // A key that is not a site id is ignored.
                }
            }
        }
        if (out.verdict.empty() && out.story.empty() && out.siteNames.empty()) {
            if (error) *error = "The model's reply had none of the expected fields.";
            return false;
        }
        if (prose) *prose = out;
        return true;
    } catch (const std::exception& e) {
        if (error) *error = std::string("The model's reply was not valid JSON: ") + e.what();
        return false;
    }
}

std::string SerializeProse(const Prose& prose) {
    jsoncons::json out;
    out["verdict"] = prose.verdict;
    out["story"] = prose.story;
    jsoncons::json sites(jsoncons::json_object_arg);
    for (const auto& entry : prose.siteNames) {
        sites.insert_or_assign(std::to_string(entry.first), entry.second);
    }
    out["sites"] = sites;
    std::string s;
    out.dump(s);
    return s;
}

bool DeserializeProse(const std::string& json, Prose* prose) {
    if (json.empty()) return false;
    std::string ignored;
    return ParseProse(json, prose, &ignored);
}

}  // namespace DebateBrief
