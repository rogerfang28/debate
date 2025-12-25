#include "DebateDatabase.h"
#include <iostream>

DebateDatabase::DebateDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool DebateDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS debates (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            user TEXT NOT NULL,
            topic TEXT NOT NULL,
            protobuf_data BLOB,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    return db_.execute(sql);
}

int DebateDatabase::addDebate(const std::string& user, const std::string& topic, const std::vector<uint8_t>& protobufData) {
    const char* sql = "INSERT INTO debates (user, topic, protobuf_data) VALUES (?, ?, ?);";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);
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

// std::vector<std::map<std::string, std::string>> DebateDatabase::getDebates(const std::string& user) {
//     std::vector<std::map<std::string, std::string>> debates;
//     const char* sql = "SELECT id, user, topic, created_at FROM debates WHERE user = ?;";
    
//     sqlite3_stmt* stmt = db_.prepare(sql);
//     if (!stmt) {
//         return debates;
//     }

//     sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);

//     while (sqlite3_step(stmt) == SQLITE_ROW) {
//         std::map<std::string, std::string> debate;
//         debate["id"] = std::to_string(sqlite3_column_int(stmt, 0));
//         debate["user"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
//         debate["topic"] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        
//         const unsigned char* timestamp = sqlite3_column_text(stmt, 3);
//         if (timestamp) {
//             debate["created_at"] = reinterpret_cast<const char*>(timestamp);
//         }
        
//         debates.push_back(debate);
//     }

//     sqlite3_finalize(stmt);
//     return debates;
// }

std::vector<uint8_t> DebateDatabase::getDebateProtobuf(int debateId) {
    std::vector<uint8_t> protobufData;
    const char* sql = "SELECT protobuf_data FROM debates WHERE id = ?;";
    
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

bool DebateDatabase::updateDebateProtobuf(int debateId, const std::string& user, const std::vector<uint8_t>& protobufData) {
    const char* sql = "UPDATE debates SET protobuf_data = ? WHERE id = ? AND user = ?;";
    
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
    sqlite3_bind_text(stmt, 3, user.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool DebateDatabase::removeDebate(int debateId, const std::string& user) {
    const char* sql = "DELETE FROM debates WHERE id = ? AND user = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, debateId);
    sqlite3_bind_text(stmt, 2, user.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool DebateDatabase::clearUserDebates(const std::string& user) {
    const char* sql = "DELETE FROM debates WHERE user = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE);

    sqlite3_finalize(stmt);
    return success;
}

bool DebateDatabase::debateExists(int debateId) {
    const char* sql = "SELECT 1 FROM debates WHERE id = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, debateId);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return exists;
}
