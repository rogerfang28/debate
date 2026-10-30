#pragma once

#include <cstdint>
#include <mutex>
#include <string>
#include <unordered_set>

#include "debate_event.pb.h"
#include "moderator_to_vr.pb.h"
#include "../../database/debate/DatabaseWrapper.h"
#include "../../utils/DebateWrapper.h"

// The site's activity log, for its admins (utils/Admins.h): who visited,
// signed in and acted, and when. Writes go to the ACTIVITY table
// (database/debate/ActivityDatabase.h); what people do inside a debate is
// already in the move log, which report() reads alongside.
//
// What is written: the time (UTC, ms), the account's id and its name at the
// time, visitor or member, what happened, and the debate / claim it was
// about. Never an IP address, a user agent, a password, an email, or what
// anyone typed into the chat. Rows are deleted after 90 days (prune, run by
// the server's periodic sweep).
//
// Logging never breaks or holds up a request: every write is one indexed
// insert, and any failure is logged and dropped.
//
// Kinds written here:
//   visit           the first request of a UTC day from an account with
//                   nothing else logged that day; a new visitor's first one
//                   says "first visit"
//   sign_in         to an existing account (detail: password / Google)
//   sign_up         a new account (detail: password / Google)
//   sign_out
//   open_debate     a debate opened (detail: "brief" from a card that opens
//                   it on its Brief, "answer" from a challenge waiting on you)
//   brief           the Brief tab opened inside a debate
//   chat            a question to the AI chat (detail: the quick prompt's
//                   name, if it was one; never the question)
//   import          an admin published an imported debate (detail: claims)
//   rename          a username changed (detail: the old name)
//   delete_account  an account closed
class ActivityLog {
public:
    ActivityLog(DatabaseWrapper& db, DebateWrapper& debateWrapper);

    static constexpr int64_t kRetentionMs = 90LL * 24 * 60 * 60 * 1000;
    static int64_t NowMs();

    // One row, as `username` (a visitor or a member by its name). `atMs` 0
    // means now.
    void record(const std::string& kind, int userId, const std::string& username, int debateId = 0,
                int claimId = 0, const std::string& detail = "", int64_t atMs = 0);

    // Someone made a request. The first time in a UTC day that an account
    // with nothing logged that day is seen, a "visit" is written; a visitor
    // account made by this very request is a "first visit".
    void noteSeen(int userId, const std::string& username, bool newVisitor = false, int64_t nowMs = 0);

    // After the moderator carried out `event` for `userId` as a request of
    // its own (not a step of an import or a walkthrough): logs the opens,
    // the Brief and the chat, if they happened.
    void recordEvent(const debate_event::DebateEvent& event, int userId);

    // A closed account: its rows lose the old name too.
    void forgetName(int userId, const std::string& closedName);

    // Deletes rows older than 90 days before `nowMs`. Returns how many.
    int prune(int64_t nowMs);

    // What the admin's Activity view shows, as of `nowMs`. `filter` picks
    // the recent list: "all", "members" or "visitors".
    moderator_to_vr::ActivityReport report(int64_t nowMs, const std::string& filter);

    static constexpr int kRecentRows = 100;
    static constexpr int kDays = 14;
    static constexpr int kTopDebates = 8;

private:
    DatabaseWrapper& db_;
    DebateWrapper& debateWrapper_;
    // Who has something logged today (UTC day number), so noteSeen asks
    // the database at most once per account per day.
    std::mutex seenMutex_;
    int64_t seenDay_ = -1;
    std::unordered_set<int> seenToday_;
};
