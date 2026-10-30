#include "ActivityLog.h"

#include <chrono>
#include <exception>
#include <map>
#include <vector>

#include "debate.pb.h"
#include "user.pb.h"
#include "../../utils/AnonymousUser.h"
#include "../../utils/Log.h"

namespace {

constexpr int64_t kDayMs = 24LL * 60 * 60 * 1000;

int64_t dayOf(int64_t ms) { return ms / kDayMs; }

ActivityDatabase::Row rowOf(const std::string& kind, int userId, const std::string& username, int debateId,
                            int claimId, const std::string& detail, int64_t atMs) {
    ActivityDatabase::Row row;
    row.atMs = atMs;
    row.userId = userId;
    row.username = username;
    row.member = !anonymous_user::IsAnonymousUsername(username);
    row.kind = kind;
    row.debateId = debateId;
    row.claimId = claimId;
    row.detail = detail;
    return row;
}

void fill(moderator_to_vr::ActivitySummary* out, const ActivityDatabase::Totals& t) {
    out->set_visitors(t.visitors);
    out->set_members(t.members);
    out->set_sign_ins(t.signIns);
    out->set_new_accounts(t.newAccounts);
    out->set_actions(t.actions);
    out->set_agrees(t.agrees);
    out->set_challenges(t.challenges);
    out->set_claims(t.claims);
    out->set_debates_opened(t.debatesOpened);
}

}  // namespace

ActivityLog::ActivityLog(DatabaseWrapper& db, DebateWrapper& debateWrapper)
    : db_(db), debateWrapper_(debateWrapper) {}

int64_t ActivityLog::NowMs() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void ActivityLog::record(const std::string& kind, int userId, const std::string& username, int debateId,
                         int claimId, const std::string& detail, int64_t atMs) {
    if (userId <= 0) return;
    if (atMs <= 0) atMs = NowMs();
    try {
        if (!db_.activity.insert(rowOf(kind, userId, username, debateId, claimId, detail, atMs))) return;
        std::lock_guard<std::mutex> lock(seenMutex_);
        if (dayOf(atMs) == seenDay_) seenToday_.insert(userId);
    } catch (const std::exception& e) {
        Log::error("[ActivityLog] Recording '" + kind + "' threw: " + e.what());
    } catch (...) {
        Log::error("[ActivityLog] Recording '" + kind + "' threw.");
    }
}

void ActivityLog::noteSeen(int userId, const std::string& username, bool newVisitor, int64_t nowMs) {
    if (userId <= 0 || username.empty()) return;
    if (nowMs <= 0) nowMs = NowMs();
    try {
        const int64_t day = dayOf(nowMs);
        std::lock_guard<std::mutex> lock(seenMutex_);
        if (day != seenDay_) {
            seenDay_ = day;
            seenToday_.clear();
        }
        if (seenToday_.count(userId)) return;
        // Once per account per day the database is asked; after that, the
        // set answers. One indexed lookup, and at most one insert.
        if (newVisitor || !db_.activity.hasRowSince(userId, day * kDayMs)) {
            db_.activity.insert(rowOf("visit", userId, username, 0, 0, newVisitor ? "first visit" : "", nowMs));
        }
        seenToday_.insert(userId);
    } catch (const std::exception& e) {
        Log::error(std::string("[ActivityLog] Noting a visit threw: ") + e.what());
    } catch (...) {
        Log::error("[ActivityLog] Noting a visit threw.");
    }
}

void ActivityLog::recordEvent(const debate_event::DebateEvent& event, int userId) {
    try {
        const debate_event::EventType type = event.type();
        if (type != debate_event::ENTER_DEBATE && type != debate_event::OPEN_DEBATE_BRIEF &&
            type != debate_event::OPEN_DEBATE_CLAIM && type != debate_event::GO_TO_BRIEF &&
            type != debate_event::ASK_CHAT) {
            return;
        }
        const user::User u = debateWrapper_.getUserProtobuf(userId);
        const bool inDebate = u.engagement().current_action() == user_engagement::ACTION_DEBATING;
        const int debateId = inDebate ? u.engagement().debating_info().debate_id() : 0;
        // It did not happen: refused, or no such debate (entering one that
        // is not there still leaves the reader "in" it).
        if (debateId <= 0 || debateWrapper_.getDebateProtobuf(debateId).empty()) return;

        switch (type) {
            case debate_event::ENTER_DEBATE:
            case debate_event::OPEN_DEBATE_BRIEF:
                if (event.enter_debate().debate_id() != debateId) return;
                record("open_debate", userId, u.username(), debateId, 0,
                       type == debate_event::OPEN_DEBATE_BRIEF ? "brief" : "");
                break;
            case debate_event::OPEN_DEBATE_CLAIM:
                if (event.open_debate_claim().debate_id() != debateId) return;
                record("open_debate", userId, u.username(), debateId, event.open_debate_claim().claim_id(), "answer");
                break;
            case debate_event::GO_TO_BRIEF:
                record("brief", userId, u.username(), debateId);
                break;
            case debate_event::ASK_CHAT:
                // Whether it was a quick prompt, by its name; never the words.
                if (event.ask_chat().question().empty() && event.ask_chat().quick_prompt().empty()) return;
                record("chat", userId, u.username(), debateId, 0, event.ask_chat().quick_prompt());
                break;
            default:
                break;
        }
    } catch (const std::exception& e) {
        Log::error(std::string("[ActivityLog] Recording an event threw: ") + e.what());
    } catch (...) {
        Log::error("[ActivityLog] Recording an event threw.");
    }
}

void ActivityLog::forgetName(int userId, const std::string& closedName) {
    if (!db_.activity.renameUser(userId, closedName)) {
        Log::warn("[ActivityLog] Could not rename the activity of user " + std::to_string(userId));
    }
}

int ActivityLog::prune(int64_t nowMs) {
    const int removed = db_.activity.deleteBefore(nowMs - kRetentionMs);
    Log::info("[ActivityLog] Removed " + std::to_string(removed) + " activity row(s) older than 90 days.");
    return removed;
}

moderator_to_vr::ActivityReport ActivityLog::report(int64_t nowMs, const std::string& filter) {
    moderator_to_vr::ActivityReport out;
    const std::string which = filter == "members" || filter == "visitors" ? filter : "all";
    out.set_generated_at_ms(nowMs);
    out.set_filter(which);

    fill(out.mutable_last_day(), db_.activity.totalsSince(nowMs - kDayMs));
    fill(out.mutable_last_week(), db_.activity.totalsSince(nowMs - 7 * kDayMs));

    // The last 14 UTC days, today first, with the empty ones as zeros.
    const int64_t todayStart = dayOf(nowMs) * kDayMs;
    std::map<std::string, ActivityDatabase::Totals> byDate;
    for (const auto& d : db_.activity.daysSince(todayStart - (kDays - 1) * kDayMs)) byDate[d.date] = d.totals;
    for (int i = 0; i < kDays; ++i) {
        const std::string date = ActivityDatabase::IsoFromMs(todayStart - i * kDayMs).substr(0, 10);
        const ActivityDatabase::Totals t = byDate.count(date) ? byDate[date] : ActivityDatabase::Totals{};
        moderator_to_vr::ActivityDay* day = out.add_days();
        day->set_date(date);
        day->set_visitors(t.visitors);
        day->set_members(t.members);
        day->set_sign_ins(t.signIns);
        day->set_new_accounts(t.newAccounts);
        day->set_actions(t.actions);
    }

    // Names and texts, looked up once each.
    std::map<int, std::string> titles, claims, names;
    auto titleOf = [&](int debateId) -> std::string {
        if (debateId <= 0) return "";
        auto it = titles.find(debateId);
        if (it != titles.end()) return it->second;
        std::string title;
        const std::vector<uint8_t> raw = debateWrapper_.getDebateProtobuf(debateId);
        debate::Debate d;
        if (!raw.empty() && d.ParseFromArray(raw.data(), static_cast<int>(raw.size()))) {
            title = debateWrapper_.getClaimById(d.root_claim_id()).sentence();
            if (title.empty()) title = d.topic();
        }
        return titles[debateId] = title;
    };
    auto claimOf = [&](int claimId) -> std::string {
        if (claimId <= 0) return "";
        auto it = claims.find(claimId);
        if (it != claims.end()) return it->second;
        return claims[claimId] = debateWrapper_.getClaimById(claimId).sentence();
    };
    auto nameOf = [&](int userId) -> std::string {
        auto it = names.find(userId);
        if (it != names.end()) return it->second;
        return names[userId] = db_.users.getUsername(userId);
    };

    for (const auto& top : db_.activity.mostOpenedSince(nowMs - 7 * kDayMs, kTopDebates)) {
        moderator_to_vr::ActivityTopDebate* t = out.add_top_debates();
        t->set_debate_id(top.debateId);
        t->set_title(titleOf(top.debateId));
        t->set_opens(top.opens);
        t->set_people(top.people);
    }

    for (const ActivityDatabase::Row& r : db_.activity.recent(which, kRecentRows, nowMs - kRetentionMs)) {
        moderator_to_vr::ActivityEntry* e = out.add_recent();
        e->set_at_ms(r.atMs);
        e->set_user_id(r.userId);
        // A debate action is in the move log by id only: named as they are now.
        e->set_username(r.username.empty() ? nameOf(r.userId) : r.username);
        e->set_member(r.username.empty() ? !anonymous_user::IsAnonymousUsername(e->username()) : r.member);
        e->set_kind(r.kind);
        e->set_debate_id(r.debateId);
        e->set_debate_title(titleOf(r.debateId));
        e->set_claim_id(r.claimId);
        e->set_claim_text(claimOf(r.claimId));
        e->set_detail(r.detail);
    }
    return out;
}
