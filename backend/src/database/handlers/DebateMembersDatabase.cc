#include "DebateMembersDatabase.h"
#include <iostream>

DebateMembersDatabase::DebateMembersDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool DebateMembersDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS DEBATE_MEMBERS (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            DEBATE_ID TEXT NOT NULL,
            USER_ID TEXT NOT NULL
        );
    )";
    return db_.execute(sql);
}

bool DebateMembersDatabase::addMember(const std::string& debateId, const std::string& userId) {
    const char* sql = "INSERT INTO DEBATE_MEMBERS (DEBATE_ID, USER_ID) VALUES (?, ?);";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, debateId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, userId.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE);

    if (!success) {
        std::cerr << "Failed to add member: " << sqlite3_errmsg(db_.handle()) << std::endl;
    }

    sqlite3_finalize(stmt);
    return success;
}

std::vector<std::string> DebateMembersDatabase::getDebateIdsForUser(const std::string& userId) {
    std::vector<std::string> debateIds;
    const char* sql = "SELECT DEBATE_ID FROM DEBATE_MEMBERS WHERE USER_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return debateIds;
    }

    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* debateId = sqlite3_column_text(stmt, 0);
        if (debateId) {
            debateIds.push_back(reinterpret_cast<const char*>(debateId));
        }
    }

    sqlite3_finalize(stmt);
    return debateIds;
}

std::vector<std::string> DebateMembersDatabase::getUserIdsForDebate(const std::string& debateId) {
    std::vector<std::string> userIds;
    const char* sql = "SELECT USER_ID FROM DEBATE_MEMBERS WHERE DEBATE_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return userIds;
    }

    sqlite3_bind_text(stmt, 1, debateId.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* userId = sqlite3_column_text(stmt, 0);
        if (userId) {
            userIds.push_back(reinterpret_cast<const char*>(userId));
        }
    }

    sqlite3_finalize(stmt);
    return userIds;
}

bool DebateMembersDatabase::isMember(const std::string& debateId, const std::string& userId) {
    const char* sql = "SELECT 1 FROM DEBATE_MEMBERS WHERE DEBATE_ID = ? AND USER_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, debateId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, userId.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return exists;
}

bool DebateMembersDatabase::removeMember(const std::string& debateId, const std::string& userId) { // ! FOR NOW ITS USING USERNAME
    const char* sql = "DELETE FROM DEBATE_MEMBERS WHERE DEBATE_ID = ? AND USER_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, debateId.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, userId.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool DebateMembersDatabase::removeAllMembersFromDebate(const std::string& debateId) {
    const char* sql = "DELETE FROM DEBATE_MEMBERS WHERE DEBATE_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, debateId.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE);

    sqlite3_finalize(stmt);
    return success;
}

bool DebateMembersDatabase::removeUserFromAllDebates(const std::string& userId) {
    const char* sql = "DELETE FROM DEBATE_MEMBERS WHERE USER_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, userId.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE);

    sqlite3_finalize(stmt);
    return success;
}
