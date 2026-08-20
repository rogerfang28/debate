#pragma once

#include "../sqlite/Database.h"
#include <string>

// ---------------------------------------
// Class: SummaryDatabase
// ---------------------------------------
// AI summaries, stored PER USER.
//
// Per user rather than per debate, deliberately. Generation is not
// deterministic, so if one summary were shared, anyone pressing Re-summarize
// would rewrite what everyone else is reading -- two people discussing "the
// summary" would no longer be discussing the same text. Isolation costs a
// model run per reader and is worth it.
//
// Kept out of the user protobuf for two reasons:
//
//   1. The user record is rewritten whole on every change. A few KB of summary
//      text riding along on every unrelated write is pure amplification -- and
//      while a run was pending, the elapsed-seconds counter was rewriting that
//      whole record every two seconds.
//
//   2. The proto held exactly one claim summary, so summarizing a second claim
//      silently destroyed the first. A table holds one row per target, so
//      clicking between claims keeps what was already generated.
//
// One row per (user, debate, kind, claim): the LATEST summary for that target.
// Re-summarizing replaces it rather than accumulating history -- nobody has
// asked to read an old summary of a debate that has since moved on.
// ---------------------------------------
class SummaryDatabase {
public:
    explicit SummaryDatabase(Database& db) : db_(db) {}

    bool ensureTable();

    struct Record {
        bool found = false;
        std::string text;
        std::string error;
        // Head of the move log when the source text was taken. Compared
        // against the debate's current head to tell the reader their summary
        // describes an older state.
        int atMoveSeq = 0;
    };

    // kind is "debate" or "claim". claimId is 0 for a debate summary.
    // Replaces any existing row for the same target.
    bool save(int userId, int debateId, const std::string& kind, int claimId,
              const std::string& text, const std::string& error, int atMoveSeq);

    Record load(int userId, int debateId, const std::string& kind, int claimId);

    bool remove(int userId, int debateId, const std::string& kind, int claimId);

private:
    Database& db_;
};
