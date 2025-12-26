#include "UserDatabase.h"
#include <iostream>

UserDatabase::UserDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool UserDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS USERS (
            USERNAME TEXT NOT NULL UNIQUE,
            USER_DATA BLOB
        );
    )";
    return db_.execute(sql);
}

int UserDatabase::createUser(const std::string& username, std::vector<uint8_t> data) {
    const char* sql = "INSERT INTO USERS (USERNAME, USER_DATA) VALUES (?, ?);";
    
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
    // This function is not used with the old schema - redirect to getUserProtobufByUsername
    return std::vector<uint8_t>();
}

std::vector<uint8_t> UserDatabase::getUserProtobufByUsername(const std::string& username) {
    std::vector<uint8_t> protobufData;
    const char* sql = "SELECT USER_DATA FROM USERS WHERE USERNAME = ?;";
    
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
    // user_id is actually the username in the old schema
    const char* sql = "UPDATE USERS SET USER_DATA = ? WHERE USERNAME = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    if (!protobufData.empty()) {
        sqlite3_bind_blob(stmt, 1, protobufData.data(), protobufData.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool UserDatabase::updateUsername(const std::string& user_id, const std::string& newUsername) {
    const char* sql = "UPDATE USERS SET USERNAME = ? WHERE USERNAME = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, newUsername.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, user_id.c_str(), -1, SQLITE_TRANSIENT);

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
    const char* sql = "DELETE FROM USERS WHERE USERNAME = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool UserDatabase::userExists(const std::string& user_id) {
    const char* sql = "SELECT 1 FROM USERS WHERE USERNAME = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, user_id.c_str(), -1, SQLITE_TRANSIENT);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return exists;
}
