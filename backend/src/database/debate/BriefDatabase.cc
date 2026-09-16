#include "BriefDatabase.h"
#include "../../utils/Log.h"

BriefDatabase::BriefDatabase(Database& db) : db_(db) {}

bool BriefDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS BRIEFS (
            DEBATE_ID   INTEGER PRIMARY KEY,
            FINGERPRINT TEXT NOT NULL,
            TEXT        TEXT NOT NULL,
            UPDATED_AT  TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
        );
    )";
    return db_.execute(sql);
}

bool BriefDatabase::save(int debateId, const std::string& fingerprint, const std::string& text) {
    if (debateId <= 0) {
        Log::warn("[BriefDatabase] Refusing to store a brief with no debate");
        return false;
    }
    sqlite3_stmt* stmt = db_.prepare(
        "INSERT INTO BRIEFS (DEBATE_ID, FINGERPRINT, TEXT, UPDATED_AT) "
        "VALUES (?, ?, ?, strftime('%Y-%m-%dT%H:%M:%fZ','now')) "
        "ON CONFLICT(DEBATE_ID) DO UPDATE SET FINGERPRINT = excluded.FINGERPRINT, "
        "TEXT = excluded.TEXT, UPDATED_AT = excluded.UPDATED_AT;");
    if (stmt == nullptr) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, debateId);
    sqlite3_bind_text(stmt, 2, fingerprint.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, text.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!ok) {
        Log::warn("[BriefDatabase] Failed to store the brief for debate " + std::to_string(debateId));
    }
    return ok;
}

bool BriefDatabase::load(int debateId, std::string* fingerprint, std::string* text) {
    sqlite3_stmt* stmt = db_.prepare(
        "SELECT FINGERPRINT, TEXT FROM BRIEFS WHERE DEBATE_ID = ?;");
    if (stmt == nullptr) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, debateId);
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        found = true;
        const unsigned char* fp = sqlite3_column_text(stmt, 0);
        const unsigned char* tx = sqlite3_column_text(stmt, 1);
        if (fingerprint) *fingerprint = fp ? reinterpret_cast<const char*>(fp) : "";
        if (text) *text = tx ? reinterpret_cast<const char*>(tx) : "";
    }
    sqlite3_finalize(stmt);
    return found;
}
