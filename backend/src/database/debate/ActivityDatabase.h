#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "../sqlite/Database.h"

// ---------------------------------------
// ActivityDatabase
// ---------------------------------------
// The site's activity log, for its admins: one row per visit, sign-in,
// sign-up, sign-out, debate opened, Brief opened, AI chat question, import,
// rename and account deletion, with who did it (their id, and their name as
// it was then), whether they were a visitor or a member, and which debate or
// claim it was about.
//
// What a person DOES in a debate (agree, challenge, support, concede,
// delete...) is not written here: the MOVES table already records it, and
// the queries below read it alongside, so the two together are the log.
//
// Never stored: IP addresses, user agents, passwords, emails, chat text.
// Rows are kept 90 days (deleteBefore, run by the server's periodic sweep).
//
// Writing must never break a request: insert() logs and returns false on any
// failure, and every caller ignores it.
// ---------------------------------------
class ActivityDatabase {
public:
    explicit ActivityDatabase(Database& db) : db_(db) {}

    // The table and its time index; also an index on MOVES.CREATED_AT, which
    // the queries read by time. Run after the MOVES table exists.
    bool ensureTable();

    struct Row {
        int64_t atMs = 0;
        int userId = 0;
        std::string username;
        bool member = false;
        std::string kind;
        int debateId = 0;
        int claimId = 0;
        std::string detail;
    };

    bool insert(const Row& row);

    // Whether `userId` has any row at or after `sinceMs`.
    bool hasRowSince(int userId, int64_t sinceMs);

    // Deletes every row older than `cutoffMs`. Returns how many.
    int deleteBefore(int64_t cutoffMs);

    // Puts `newName` on every row of `userId` (a closed account keeps no old
    // name here either).
    bool renameUser(int userId, const std::string& newName);

    // ---- Reading: ACTIVITY and the debate actions in MOVES, together ----

    struct Totals {
        int visitors = 0;      // distinct visitor ids
        int members = 0;       // distinct member ids
        int signIns = 0;
        int newAccounts = 0;
        int actions = 0;
        int agrees = 0;
        int challenges = 0;
        int claims = 0;        // supports and new debates
        int debatesOpened = 0;
    };
    // Everything from `sinceMs` on.
    Totals totalsSince(int64_t sinceMs);

    struct Day {
        std::string date;      // "YYYY-MM-DD", UTC
        Totals totals;
    };
    // One entry per UTC day that has anything, from `sinceMs` on.
    std::vector<Day> daysSince(int64_t sinceMs);

    struct TopDebate {
        int debateId = 0;
        int opens = 0;
        int people = 0;
    };
    // The debates opened most from `sinceMs` on, most first.
    std::vector<TopDebate> mostOpenedSince(int64_t sinceMs, int limit);

    // The newest rows from `sinceMs` on, newest first: "all", "members" or
    // "visitors". Debate actions come from MOVES with kind new_debate,
    // support, challenge, agree, withdraw, concede or delete, member = true
    // and username empty (the caller names the actor).
    std::vector<Row> recent(const std::string& filter, int limit, int64_t sinceMs);

    // "2026-10-29T14:03:00.000Z": the form MOVES.CREATED_AT is written in.
    static std::string IsoFromMs(int64_t ms);

private:
    Database& db_;
};
