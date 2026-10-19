#include "MoveBackfill.h"

#include "../statusEngine/StatusEngine.h"
#include "../utils/ClaimVoice.h"
#include "../utils/Log.h"

#include <algorithm>
#include <map>
#include <set>
#include <tuple>

namespace {

const char* kMark = "\"backfilled\":1";

struct LinkRow {
    int id = 0;
    int from = 0;
    int to = 0;
    int creator = 0;
    int type = 0;
    int relationId = 0;
};

std::string opposePayload(int relationId, int challengingClaimId, int voiceId) {
    std::string p = "{\"relation_id\":" + std::to_string(relationId) +
                    ",\"challenging_claim_id\":" + std::to_string(challengingClaimId);
    if (voiceId > 0) p += ",\"voice_id\":" + std::to_string(voiceId);
    return p + "," + kMark + "}";
}

}  // namespace

namespace MoveBackfill {

Report BackfillDebate(int debateId, DebateWrapper& debateWrapper, DatabaseWrapper& db) {
    Report report;
    if (!debateWrapper.getMovesForDebate(debateId).empty()) {
        return report;  // has a real log; nothing to invent
    }

    // ---- The graph, as stored ----
    std::vector<debate::Claim> claims;
    for (const auto& bytes : debateWrapper.getStatementsForDebate(debateId)) {
        debate::Claim c;
        if (c.ParseFromArray(bytes.data(), static_cast<int>(bytes.size())) && c.id() > 0) {
            claims.push_back(c);
        }
    }
    if (claims.empty()) {
        return report;
    }
    std::sort(claims.begin(), claims.end(),
              [](const debate::Claim& a, const debate::Claim& b) { return a.id() < b.id(); });

    std::vector<LinkRow> rows;
    for (const auto& t : debateWrapper.getLinksForDebate(debateId)) {
        LinkRow r;
        r.id = std::get<0>(t);
        r.from = std::get<1>(t);
        r.to = std::get<2>(t);
        r.creator = std::get<4>(t);
        r.type = std::get<5>(t);
        r.relationId = std::get<6>(t);
        rows.push_back(r);
    }
    std::sort(rows.begin(), rows.end(), [](const LinkRow& a, const LinkRow& b) { return a.id < b.id; });
    const std::map<int, int> relationTargets = debateWrapper.getRelationTargets(debateId);

    std::map<int, const debate::Claim*> claimById;
    for (const auto& c : claims) claimById[c.id()] = &c;

    auto write = [&](int actor, const char* type, bool targetIsClaim, int target,
                     const std::string& payload) {
        if (actor <= 0 || target <= 0) return;
        if (db.moves.appendMove(debateId, actor, type, targetIsClaim ? "claim" : "relation",
                                target, payload)) {
            ++report.movesWritten;
        }
    };

    // ---- What the graph itself says happened ----
    for (const auto& c : claims) {
        write(c.creator_id() > 0 ? c.creator_id() : ClaimVoiceId(c), "ASSERT", true, c.id(),
              std::string("{") + kMark + "}");
    }
    for (const LinkRow& r : rows) {
        if (r.type == static_cast<int>(debate::LinkType::PARENT_CHILD)) {
            // One ASSERT per relation, not per row of a joint relation.
            const int relationId = r.relationId > 0 ? r.relationId : r.id;
            if (relationId != r.id) continue;
            write(r.creator, "ASSERT", false, relationId,
                  "{\"from_claim_id\":" + std::to_string(r.from) +
                  ",\"to_claim_ids\":[" + std::to_string(r.to) + "]," + kMark + "}");
        } else if (r.type == static_cast<int>(debate::LinkType::CHALLENGE)) {
            auto cIt = claimById.find(r.from);
            const int voice = cIt == claimById.end() ? 0 : cIt->second->voice_id();
            const int actor = cIt == claimById.end() ? r.creator : cIt->second->creator_id();
            auto tIt = relationTargets.find(r.id);
            if (tIt != relationTargets.end() && tIt->second > 0) {
                write(actor, "OPPOSE", false, tIt->second, opposePayload(r.id, r.from, voice));
            } else {
                write(actor, "OPPOSE", true, r.to, opposePayload(r.id, r.from, voice));
            }
        }
    }

    // ---- What the stored statuses say beyond that ----
    std::map<int, std::string> usernames;
    std::map<std::string, int> userIds;
    auto nameOf = [&](int userId) -> const std::string& {
        auto it = usernames.find(userId);
        if (it == usernames.end()) {
            it = usernames.emplace(userId, debateWrapper.getUserProtobuf(userId).username()).first;
            if (!it->second.empty()) userIds[it->second] = userId;
        }
        return it->second;
    };
    for (const auto& c : claims) {
        nameOf(c.creator_id());
        if (c.voice_id() > 0) nameOf(c.voice_id());
        for (const auto& e : c.user_statuses()) {
            if (!userIds.count(e.first)) {
                const int id = db.users.getUserId(e.first);
                if (id > 0) { usernames[id] = e.first; userIds[e.first] = id; }
            }
        }
    }
    std::vector<debate::Relationship> relations;
    for (const LinkRow& r : rows) relations.push_back(debateWrapper.getLinkById(r.id));

    auto replay = [&]() {
        return StatusEngine::computeLegacyStatuses(
            claims, relations, debateWrapper.getMovesForDebate(debateId), usernames);
    };
    auto view = [](const StatusEngine::LegacyResult& r, const std::string& user, int claimId,
                   bool* present) {
        auto u = r.by_user.find(user);
        if (u == r.by_user.end()) { *present = false; return debate::ClaimStatus::UNDETERMINED; }
        auto c = u->second.find(claimId);
        if (c == u->second.end()) { *present = false; return debate::ClaimStatus::UNDETERMINED; }
        *present = true;
        return c->second;
    };

    // Two passes: the first writes what the differences call for, the second
    // reports what is still different once those are replayed too.
    for (int pass = 0; pass < 2; ++pass) {
        const StatusEngine::LegacyResult computed = replay();
        for (const auto& c : claims) {
            const std::string holder = nameOf(ClaimVoiceId(c));
            for (const auto& e : c.user_statuses()) {
                const std::string& user = e.first;
                auto idIt = userIds.find(user);
                if (idIt == userIds.end()) continue;  // a name that is nobody
                bool present = false;
                const debate::ClaimStatus got = view(computed, user, c.id(), &present);
                if (present && got == e.second) continue;
                if (e.second == debate::ClaimStatus::UNDETERMINED) continue;  // nothing to say

                if (pass == 1) {
                    report.unrepresentable.push_back(
                        "debate " + std::to_string(debateId) + " claim " + std::to_string(c.id()) +
                        " user " + user + ": stored " +
                        (e.second == debate::ClaimStatus::TRUE_CLAIM ? "TRUE" : "FALSE") +
                        ", replay gives " + (!present ? "nothing" :
                            got == debate::ClaimStatus::TRUE_CLAIM ? "TRUE" :
                            got == debate::ClaimStatus::FALSE_CLAIM ? "FALSE" : "UNDETERMINED"));
                    continue;
                }

                if (e.second == debate::ClaimStatus::TRUE_CLAIM) {
                    // Holds it true without having made it: that is an ACCEPT.
                    write(idIt->second, "ACCEPT", true, c.id(), std::string("{") + kMark + "}");
                } else if (user == holder) {
                    // The author holds their own claim FALSE: they conceded a
                    // challenge on it. The latest challenge row is the one.
                    int challengeRow = 0, challenger = 0;
                    for (const LinkRow& r : rows) {
                        if (r.type == static_cast<int>(debate::LinkType::CHALLENGE) && r.to == c.id() &&
                            !relationTargets.count(r.id) && r.id > challengeRow) {
                            challengeRow = r.id;
                            challenger = r.from;
                        }
                    }
                    if (challengeRow > 0) {
                        write(idIt->second, "CONCEDE", true, c.id(),
                              "{\"challenge_relation_id\":" + std::to_string(challengeRow) +
                              ",\"challenging_claim_id\":" + std::to_string(challenger) + "," + kMark + "}");
                    }
                }
                // A FALSE held by someone who neither made the claim nor
                // challenged it is derived, or unrepresentable: pass 2 reports it.
            }
        }
    }

    report.debatesTouched = 1;
    Log::info("[MoveBackfill] debate " + std::to_string(debateId) + ": wrote " +
              std::to_string(report.movesWritten) + " moves; " +
              std::to_string(report.unrepresentable.size()) + " stored statuses no move can express");
    for (const auto& line : report.unrepresentable) {
        Log::info("[MoveBackfill]   " + line);
    }
    return report;
}

Report BackfillAll(DebateWrapper& debateWrapper, DatabaseWrapper& db) {
    Report total;
    for (const int debateId : db.debates.allDebateIds()) {
        const Report r = BackfillDebate(debateId, debateWrapper, db);
        total.debatesTouched += r.debatesTouched;
        total.movesWritten += r.movesWritten;
        total.unrepresentable.insert(total.unrepresentable.end(),
                                     r.unrepresentable.begin(), r.unrepresentable.end());
    }
    if (total.debatesTouched > 0) {
        Log::info("[MoveBackfill] " + std::to_string(total.debatesTouched) + " debate(s) had no move log; wrote " +
                  std::to_string(total.movesWritten) + " moves");
    }
    return total;
}

}  // namespace MoveBackfill
