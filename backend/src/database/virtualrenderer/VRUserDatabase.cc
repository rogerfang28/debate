#include "VRUserDatabase.h"
#include <iostream>
#include "../../../src/gen/cpp/user.pb.h"

VRUserDatabase::VRUserDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool VRUserDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS USERS (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            USERNAME TEXT NOT NULL UNIQUE,
            USER_DATA BLOB
        );
    )";
    return db_.execute(sql);
}

int VRUserDatabase::createUser(const std::string& username, std::vector<uint8_t> data) {
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

std::vector<uint8_t> VRUserDatabase::getUserProtobuf(int user_id) {
    std::vector<uint8_t> protobufData;
    const char* sql = "SELECT USER_DATA FROM USERS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return protobufData;
    }

    sqlite3_bind_int(stmt, 1, user_id);

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

std::vector<uint8_t> VRUserDatabase::getUserProtobufByUsername(const std::string& username) {
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

bool VRUserDatabase::updateUserProtobuf(int user_id, const std::vector<uint8_t>& protobufData) {
    const char* sql = "UPDATE USERS SET USER_DATA = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    if (!protobufData.empty()) {
        sqlite3_bind_blob(stmt, 1, protobufData.data(), protobufData.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_int(stmt, 2, user_id);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool VRUserDatabase::updateUsername(int user_id, const std::string& newUsername) {
    const char* sql = "UPDATE USERS SET USERNAME = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, newUsername.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, user_id);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool VRUserDatabase::updateUserPassword(int user_id, const std::string& newPasswordHash) {
    // Password lives inside the serialized User protobuf (USER_DATA blob),
    // same pattern as updateGoogleSub: load, set the field, save back.
    std::vector<uint8_t> protobufData = getUserProtobuf(user_id);
    if (protobufData.empty()) {
        return false;
    }

    user::User newUser;
    if (!newUser.ParseFromArray(protobufData.data(), static_cast<int>(protobufData.size()))) {
        return false;
    }

    newUser.set_password_hash(newPasswordHash);

    std::string serializedData;
    newUser.SerializeToString(&serializedData);

    return updateUserProtobuf(user_id, std::vector<uint8_t>(
        serializedData.begin(), serializedData.end()
    ));
}

bool VRUserDatabase::updateGoogleSub(int user_id, const std::string& google_sub, const std::string& email) {
    // Fetch existing protobuf data
    std::vector<uint8_t> protobufData = getUserProtobuf(user_id);
    if (protobufData.empty()) {
        return false;
    }

    // Parse the User protobuf
    user::User newUser;
    if (!newUser.ParseFromArray(protobufData.data(), static_cast<int>(protobufData.size()))) {
        return false;
    }

    // Update google_sub and email in the protobuf
    newUser.set_google_sub(google_sub);
    newUser.set_email(email);

    // Serialize back to blob
    std::string serializedData;
    newUser.SerializeToString(&serializedData);

    return updateUserProtobuf(user_id, std::vector<uint8_t>(
        serializedData.begin(), serializedData.end()
    ));
}

bool VRUserDatabase::deleteUser(int user_id) {
    const char* sql = "DELETE FROM USERS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool VRUserDatabase::userExists(int user_id) {
    const char* sql = "SELECT 1 FROM USERS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, user_id);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return exists;
}

std::string VRUserDatabase::getUsername(int id) {
    std::string username;
    const char* sql = "SELECT USERNAME FROM USERS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return username;
    }

    sqlite3_bind_int(stmt, 1, id);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) {
            username = reinterpret_cast<const char*>(text);
        }
    }

    sqlite3_finalize(stmt);
    return username;
}

int VRUserDatabase::getUserId(const std::string& username) {
    int userId = -1;
    const char* sql = "SELECT ID FROM USERS WHERE USERNAME = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return userId;
    }

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        userId = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return userId;
}
