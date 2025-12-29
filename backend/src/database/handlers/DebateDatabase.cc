#include "DebateDatabase.h"
#include <iostream>

DebateDatabase::DebateDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool DebateDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS DEBATE (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            USER_ID INTEGER NOT NULL,
            TOPIC TEXT NOT NULL,
            PAGE_DATA BLOB
        );
    )";
    return db_.execute(sql);
}

int DebateDatabase::addDebate(int userId, const std::string& topic, const std::vector<uint8_t>& protobufData) {
    const char* sql = "INSERT INTO DEBATE (USER_ID, TOPIC, PAGE_DATA) VALUES (?, ?, ?);";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, userId);
    sqlite3_bind_text(stmt, 2, topic.c_str(), -1, SQLITE_TRANSIENT);
    
    if (!protobufData.empty()) {
        sqlite3_bind_blob(stmt, 3, protobufData.data(), protobufData.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }

    int result = sqlite3_step(stmt);
    int debateId = -1;
    
    if (result == SQLITE_DONE) {
        debateId = static_cast<int>(sqlite3_last_insert_rowid(db_.handle()));
    } else {
        std::cerr << "Failed to insert debate: " << sqlite3_errmsg(db_.handle()) << std::endl;
    }

    sqlite3_finalize(stmt);
    return debateId;
}

std::vector<uint8_t> DebateDatabase::getDebateProtobuf(int debateId) {
    std::vector<uint8_t> protobufData;
    const char* sql = "SELECT PAGE_DATA FROM DEBATE WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return protobufData;
    }

    sqlite3_bind_int(stmt, 1, debateId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int blobSize = sqlite3_column_bytes(stmt, 0);
        
        if (blob && blobSize > 0) {
            const uint8_t* data = static_cast<const uint8_t*>(blob);
            protobufData.assign(data, data + blobSize);
        }
    }

    sqlite3_finalize(stmt);
    return protobufData;
}

bool DebateDatabase::updateDebateProtobuf(int debateId, int userId, const std::vector<uint8_t>& protobufData) {
    const char* sql = "UPDATE DEBATE SET PAGE_DATA = ? WHERE ID = ? AND USER_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    if (!protobufData.empty()) {
        sqlite3_bind_blob(stmt, 1, protobufData.data(), protobufData.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_int(stmt, 2, debateId);
    sqlite3_bind_int(stmt, 3, userId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool DebateDatabase::removeDebate(int debateId, int userId) {
    const char* sql = "DELETE FROM DEBATE WHERE ID = ? AND USER_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, debateId);
    sqlite3_bind_int(stmt, 2, userId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool DebateDatabase::clearUserDebates(int userId) {
    const char* sql = "DELETE FROM DEBATE WHERE USER_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, userId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE);

    sqlite3_finalize(stmt);
    return success;
}

bool DebateDatabase::debateExists(int debateId) {
    const char* sql = "SELECT 1 FROM DEBATE WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, debateId);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return exists;
}
