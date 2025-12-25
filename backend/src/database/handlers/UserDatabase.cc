#include "UserDatabase.h"
#include <iostream>

UserDatabase::UserDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool UserDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS users (
            user_id INTEGER PRIMARY KEY AUTOINCREMENT,
            username TEXT UNIQUE NOT NULL,
            protobuf_data BLOB,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        );
    )";
    return db_.execute(sql);
}

int UserDatabase::createUser(const std::string& username, std::vector<uint8_t> data) {
    const char* sql = "INSERT INTO users (username, protobuf_data) VALUES (?, ?);";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    
    if (!data.empty()) {
        sqlite3_bind_blob(stmt, 2, data.data(), data.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 2);
    }

    int result = sqlite3_step(stmt);
    int userId = -1;
    
    if (result == SQLITE_DONE) {
        userId = static_cast<int>(sqlite3_last_insert_rowid(db_.handle()));
    } else {
        std::cerr << "Failed to create user: " << sqlite3_errmsg(db_.handle()) << std::endl;
    }

    sqlite3_finalize(stmt);
    return userId;
}

std::vector<uint8_t> UserDatabase::getUserProtobuf(const std::string& user_id) {
    std::vector<uint8_t> protobufData;
    const char* sql = "SELECT protobuf_data FROM users WHERE user_id = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return protobufData;
    }

    sqlite3_bind_int(stmt, 1, std::stoi(user_id));

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

std::vector<uint8_t> UserDatabase::getUserProtobufByUsername(const std::string& username) {
    std::vector<uint8_t> protobufData;
    const char* sql = "SELECT protobuf_data FROM users WHERE username = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return protobufData;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

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

bool UserDatabase::updateUserProtobuf(const std::string& user_id, const std::vector<uint8_t>& protobufData) {
    const char* sql = "UPDATE users SET protobuf_data = ? WHERE user_id = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    if (!protobufData.empty()) {
        sqlite3_bind_blob(stmt, 1, protobufData.data(), protobufData.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_int(stmt, 2, std::stoi(user_id));

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool UserDatabase::updateUsername(const std::string& user_id, const std::string& newUsername) {
    const char* sql = "UPDATE users SET username = ? WHERE user_id = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, newUsername.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, std::stoi(user_id));

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool UserDatabase::updateUserPassword(const std::string& user_id, const std::string& newPasswordHash) {
    // Note: Password hash should be stored in protobuf data
    // This is a placeholder implementation
    std::cerr << "updateUserPassword: Password should be managed via protobuf data" << std::endl;
    return false;
}

bool UserDatabase::deleteUser(const std::string& user_id) {
    const char* sql = "DELETE FROM users WHERE user_id = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, std::stoi(user_id));

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool UserDatabase::userExists(const std::string& user_id) {
    const char* sql = "SELECT 1 FROM users WHERE user_id = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, std::stoi(user_id));

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return exists;
}
