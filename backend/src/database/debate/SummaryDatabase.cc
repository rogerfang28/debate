#include "SummaryDatabase.h"
#include "../../utils/Log.h"

bool SummaryDatabase::ensureTable() {
    // The UNIQUE key is the whole design: one row per target per user, so a
    // re-summarize REPLACEs rather than appends, and two users summarizing the
    // same claim never touch each other's row.
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS SUMMARIES (
            ID          INTEGER PRIMARY KEY AUTOINCREMENT,
            USER_ID     INTEGER NOT NULL,
            DEBATE_ID   INTEGER NOT NULL,
            KIND        TEXT NOT NULL,
            CLAIM_ID    INTEGER NOT NULL DEFAULT 0,
            TEXT        TEXT,
            ERROR       TEXT,
            AT_MOVE_SEQ INTEGER NOT NULL DEFAULT 0,
            CREATED_AT  TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now')),
            UNIQUE (USER_ID, DEBATE_ID, KIND, CLAIM_ID)
        );
    )";
    if (!db_.execute(sql)) {
        return false;
    }
    return db_.execute(
        "CREATE INDEX IF NOT EXISTS IDX_SUMMARIES_LOOKUP "
        "ON SUMMARIES(USER_ID, DEBATE_ID, KIND, CLAIM_ID);");
}

bool SummaryDatabase::save(int userId, int debateId, const std::string& kind, int claimId,
                           const std::string& text, const std::string& error, int atMoveSeq) {
    if (userId <= 0 || debateId <= 0) {
        Log::warn("[SummaryDatabase] Refusing to save a summary with no user or debate");
        return false;
    }

    const char* sql =
        "INSERT INTO SUMMARIES (USER_ID, DEBATE_ID, KIND, CLAIM_ID, TEXT, ERROR, AT_MOVE_SEQ) "
        "VALUES (?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(USER_ID, DEBATE_ID, KIND, CLAIM_ID) DO UPDATE SET "
        "TEXT = excluded.TEXT, ERROR = excluded.ERROR, AT_MOVE_SEQ = excluded.AT_MOVE_SEQ, "
        "CREATED_AT = strftime('%Y-%m-%dT%H:%M:%fZ','now');";
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (stmt == nullptr) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, debateId);
    sqlite3_bind_text(stmt, 3, kind.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, claimId);
    sqlite3_bind_text(stmt, 5, text.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 6, error.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 7, atMoveSeq);

    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!ok) {
        Log::warn("[SummaryDatabase] Failed to save summary for user " + std::to_string(userId));
    }
    return ok;
}

SummaryDatabase::Record SummaryDatabase::load(int userId, int debateId,
                                              const std::string& kind, int claimId) {
    Record record;
    if (userId <= 0 || debateId <= 0) {
        return record;
    }

    const char* sql =
        "SELECT TEXT, ERROR, AT_MOVE_SEQ FROM SUMMARIES "
        "WHERE USER_ID = ? AND DEBATE_ID = ? AND KIND = ? AND CLAIM_ID = ?;";
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (stmt == nullptr) {
        return record;
    }

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, debateId);
    sqlite3_bind_text(stmt, 3, kind.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, claimId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        record.found = true;
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        const unsigned char* error = sqlite3_column_text(stmt, 1);
        record.text = text ? reinterpret_cast<const char*>(text) : "";
        record.error = error ? reinterpret_cast<const char*>(error) : "";
        record.atMoveSeq = sqlite3_column_int(stmt, 2);
    }
    sqlite3_finalize(stmt);
    return record;
}

bool SummaryDatabase::remove(int userId, int debateId, const std::string& kind, int claimId) {
    const char* sql =
        "DELETE FROM SUMMARIES WHERE USER_ID = ? AND DEBATE_ID = ? AND KIND = ? AND CLAIM_ID = ?;";
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (stmt == nullptr) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, debateId);
    sqlite3_bind_text(stmt, 3, kind.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, claimId);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}
