#include "ChatDatabase.h"
#include "../../utils/Log.h"

ChatDatabase::ChatDatabase(Database& db) : db_(db) {}

bool ChatDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS CHATS (
            ID         INTEGER PRIMARY KEY AUTOINCREMENT,
            USER_ID    INTEGER NOT NULL,
            DEBATE_ID  INTEGER NOT NULL,
            ROLE       TEXT NOT NULL,
            TEXT       TEXT NOT NULL,
            CREATED_AT TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%fZ','now'))
        );
    )";
    if (!db_.execute(sql)) {
        return false;
    }
    return db_.execute(
        "CREATE INDEX IF NOT EXISTS IDX_CHATS_LOOKUP ON CHATS(USER_ID, DEBATE_ID, ID);");
}

bool ChatDatabase::append(int userId, int debateId, const std::string& role,
                          const std::string& text) {
    if (userId <= 0 || debateId <= 0) {
        Log::warn("[ChatDatabase] Refusing to store a message with no user or debate");
        return false;
    }
    sqlite3_stmt* stmt = db_.prepare(
        "INSERT INTO CHATS (USER_ID, DEBATE_ID, ROLE, TEXT) VALUES (?, ?, ?, ?);");
    if (stmt == nullptr) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, debateId);
    sqlite3_bind_text(stmt, 3, role.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, text.c_str(), -1, SQLITE_TRANSIENT);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    if (!ok) {
        Log::warn("[ChatDatabase] Failed to store a message for user " + std::to_string(userId));
    }
    return ok;
}

std::vector<ChatDatabase::Message> ChatDatabase::list(int userId, int debateId) {
    std::vector<Message> out;
    sqlite3_stmt* stmt = db_.prepare(
        "SELECT ROLE, TEXT FROM CHATS WHERE USER_ID = ? AND DEBATE_ID = ? ORDER BY ID;");
    if (stmt == nullptr) {
        return out;
    }
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, debateId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Message m;
        const unsigned char* role = sqlite3_column_text(stmt, 0);
        const unsigned char* text = sqlite3_column_text(stmt, 1);
        m.role = role ? reinterpret_cast<const char*>(role) : "";
        m.text = text ? reinterpret_cast<const char*>(text) : "";
        out.push_back(std::move(m));
    }
    sqlite3_finalize(stmt);
    return out;
}

bool ChatDatabase::clear(int userId, int debateId) {
    sqlite3_stmt* stmt = db_.prepare("DELETE FROM CHATS WHERE USER_ID = ? AND DEBATE_ID = ?;");
    if (stmt == nullptr) {
        return false;
    }
    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_int(stmt, 2, debateId);
    const bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    sqlite3_finalize(stmt);
    return ok;
}
