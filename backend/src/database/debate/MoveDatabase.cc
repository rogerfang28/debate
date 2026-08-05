#include "MoveDatabase.h"
#include "../../utils/Log.h"

bool MoveDatabase::ensureTable() {
    // SEQ is monotonic per debate, not globally. The UNIQUE constraint is what
    // actually guarantees that -- if two writers ever computed the same seq,
    // the second insert fails loudly instead of silently producing a log with
    // two "move 7"s in one debate, which would be unorderable afterwards.
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS MOVES (
            ID          INTEGER PRIMARY KEY AUTOINCREMENT,
            DEBATE_ID   INTEGER NOT NULL,
            SEQ         INTEGER NOT NULL,
            ACTOR_ID    INTEGER NOT NULL,
            TYPE        TEXT NOT NULL,
            TARGET_TYPE TEXT NOT NULL,
            TARGET_ID   INTEGER NOT NULL,
            PAYLOAD     TEXT,
            CREATED_AT  TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
            UNIQUE (DEBATE_ID, SEQ)
        );
    )";
    if (!db_.execute(sql)) {
        return false;
    }

    // Replay order for one debate is always (DEBATE_ID, SEQ).
    if (!db_.execute("CREATE INDEX IF NOT EXISTS IDX_MOVES_DEBATE_SEQ ON MOVES(DEBATE_ID, SEQ);")) {
        return false;
    }

    return true;
}

bool MoveDatabase::appendMove(int debateId,
                              int actorId,
                              const std::string& type,
                              const std::string& targetType,
                              int targetId,
                              const std::string& payload) {
    if (debateId <= 0) {
        // Without a debate we cannot order the move, and an unordered move is
        // worse than none: it would look like part of some debate's sequence.
        Log::warn("[MoveDatabase] Refusing to log move '" + type +
                  "' with no debate id (target " + targetType + " " +
                  std::to_string(targetId) + ")");
        return false;
    }

    // seq is computed inside the INSERT rather than by a separate SELECT, so
    // there is no window in which another writer could claim the same seq.
    // Database serialises individual statements but not statement pairs.
    const char* sql =
        "INSERT INTO MOVES (DEBATE_ID, SEQ, ACTOR_ID, TYPE, TARGET_TYPE, TARGET_ID, PAYLOAD) "
        "SELECT ?, COALESCE(MAX(SEQ), 0) + 1, ?, ?, ?, ?, ? FROM MOVES WHERE DEBATE_ID = ?;";

    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        Log::error("[MoveDatabase] Failed to prepare move insert for debate " +
                   std::to_string(debateId));
        return false;
    }

    sqlite3_bind_int(stmt, 1, debateId);
    sqlite3_bind_int(stmt, 2, actorId);
    sqlite3_bind_text(stmt, 3, type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, targetType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, targetId);
    sqlite3_bind_text(stmt, 6, payload.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, debateId);

    const int result = sqlite3_step(stmt);
    const bool ok = (result == SQLITE_DONE);
    if (!ok) {
        Log::error("[MoveDatabase] Failed to append move '" + type + "' for debate " +
                   std::to_string(debateId) + ": " +
                   std::string(sqlite3_errmsg(db_.handle())));
    }

    sqlite3_finalize(stmt);
    return ok;
}

int MoveDatabase::countMovesForDebate(int debateId) {
    sqlite3_stmt* stmt = db_.prepare("SELECT COUNT(*) FROM MOVES WHERE DEBATE_ID = ?;");
    if (!stmt) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, debateId);
    int count = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return count;
}

namespace {

// TYPE and TARGET_TYPE are stored as text so the log stays readable years
// later and survives enum renumbering. Unknown names map to UNSPECIFIED rather
// than being guessed at -- a move whose type we cannot read must not be
// silently treated as some other move.
debate::MoveType moveTypeFromName(const std::string& s) {
    if (s == "ASSERT")       return debate::ASSERT;
    if (s == "OPPOSE")       return debate::OPPOSE;
    if (s == "CONCEDE")      return debate::CONCEDE;
    if (s == "ACCEPT")       return debate::ACCEPT;
    if (s == "REST")         return debate::REST;
    if (s == "FLAG_UNKNOWN") return debate::FLAG_UNKNOWN;
    if (s == "REPLACE")      return debate::REPLACE;
    if (s == "DISOWN")       return debate::DISOWN;
    if (s == "REOPEN")       return debate::REOPEN;
    if (s == "TIMEOUT")      return debate::TIMEOUT;
    if (s == "RETRACT")      return debate::RETRACT;
    if (s == "REDACT")       return debate::REDACT;
    if (s == "CONTRADICT")   return debate::CONTRADICT;
    return debate::MOVE_TYPE_UNSPECIFIED;
}

debate::TargetType targetTypeFromName(const std::string& s) {
    if (s == "claim")    return debate::TARGET_CLAIM;
    if (s == "relation") return debate::TARGET_RELATION;
    return debate::TARGET_TYPE_UNSPECIFIED;
}

}  // namespace

std::vector<debate::Move> MoveDatabase::getMovesForDebate(int debateId) {
    std::vector<debate::Move> moves;
    sqlite3_stmt* stmt = db_.prepare(
        "SELECT ID, SEQ, ACTOR_ID, TYPE, TARGET_TYPE, TARGET_ID, IFNULL(PAYLOAD,'') "
        "FROM MOVES WHERE DEBATE_ID = ? ORDER BY SEQ;");
    if (!stmt) {
        Log::error("[MoveDatabase] Failed to prepare move read for debate " +
                   std::to_string(debateId));
        return moves;
    }
    sqlite3_bind_int(stmt, 1, debateId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        debate::Move m;
        m.set_id(sqlite3_column_int(stmt, 0));
        m.set_debate_id(debateId);
        m.set_seq(sqlite3_column_int(stmt, 1));
        m.set_actor_id(sqlite3_column_int(stmt, 2));
        m.set_type(moveTypeFromName(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3))));
        m.set_target_type(targetTypeFromName(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4))));
        m.set_target_id(sqlite3_column_int(stmt, 5));
        m.set_payload(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6)));
        moves.push_back(std::move(m));
    }

    sqlite3_finalize(stmt);
    return moves;
}

int MoveDatabase::lastSeqForDebate(int debateId) {
    sqlite3_stmt* stmt = db_.prepare("SELECT COALESCE(MAX(SEQ), 0) FROM MOVES WHERE DEBATE_ID = ?;");
    if (!stmt) {
        return -1;
    }
    sqlite3_bind_int(stmt, 1, debateId);
    int seq = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        seq = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return seq;
}
