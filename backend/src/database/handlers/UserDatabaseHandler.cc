#include "UserDatabaseHandler.h"
#include <iostream>
#include <vector>
#include <cstdint>  // for std::vector<uint8_t>
#include "../sqlite/DatabaseCommunicator.h"

UserDatabaseHandler::UserDatabaseHandler(const std::string& dbFile)
    : dbFilename(dbFile) {
    ensureTable();
}

// ---------------------------
// Create Table
// ---------------------------

void UserDatabaseHandler::ensureTable() {
    if (!openDB(dbFilename)) return;

    // Drop old table if it exists with old schema
    // execSQL("DROP TABLE IF EXISTS USERS;");

    createTable("USERS", {
        {"USERNAME", "TEXT NOT NULL UNIQUE"},
        {"USER_DATA", "BLOB"}  // serialized user.proto protobuf
    });

    closeDB();
}

// ---------------------------
// Add Users
// ---------------------------

int UserDatabaseHandler::addUser(const std::string& username,
                                 const std::vector<uint8_t>& protobufData) {
    if (!openDB(dbFilename)) return -1;

    std::vector<std::pair<const void*, int>> blobValues;
    if (!protobufData.empty())
        blobValues.push_back({protobufData.data(), static_cast<int>(protobufData.size())});
    else
        blobValues.push_back({nullptr, 0});

    int id = insertRowWithBlob(
        "USERS",
        {"USERNAME", "USER_DATA"},
        {username},
        blobValues
    );

    closeDB();

    if (id != -1)
        std::cout << "[UserDB] Added user: " << username
                  << " [data size: " << protobufData.size() << " bytes]\n";

    return id;
}

// ---------------------------
// Retrieve Users
// ---------------------------

std::vector<std::map<std::string, std::string>>
UserDatabaseHandler::getAllUsers() {
    if (!openDB(dbFilename)) return {};
    auto rows = readRows("USERS");
    closeDB();

    std::cout << "[UserDB] Retrieved " << rows.size() << " users.\n";
    return rows;
}

std::vector<uint8_t>
UserDatabaseHandler::getUserProtobuf(const std::string& username) {
    if (!openDB(dbFilename)) return {};
    auto blob = readBlob("USERS", "USER_DATA", "USERNAME = '" + username + "'");
    closeDB();

    std::cout << "[UserDB] Retrieved protobuf data for user " << username
              << " (size: " << blob.size() << " bytes)\n";
    return blob;
}

bool UserDatabaseHandler::userExists(const std::string& username) {
    if (!openDB(dbFilename)) return false;
    auto rows = readRows("USERS", "USERNAME = '" + username + "'");
    closeDB();

    return !rows.empty();
}

// ---------------------------
// Update / Delete
// ---------------------------

bool UserDatabaseHandler::updateUserProtobuf(const std::string& username,
                                             const std::vector<uint8_t>& protobufData) {
    std::cout << "[UserDB] Updating protobuf data for user " << username
              << " (size: " << protobufData.size() << " bytes)\n";

    if (!openDB(dbFilename)) return false;
    bool ok = updateRowWithBlob("USERS", "USER_DATA", protobufData,
                                "USERNAME = '" + username + "'");
    closeDB();

    if (ok)
        std::cout << "[UserDB] Updated protobuf data for " << username << "\n";
    return ok;
}

bool UserDatabaseHandler::removeUser(const std::string& username) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("USERS", "USERNAME = '" + username + "'");
    closeDB();

    if (ok)
        std::cout << "[UserDB] Removed user: " << username << "\n";
    return ok;
}

bool UserDatabaseHandler::clearAllUsers() {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("USERS");
    closeDB();

    if (ok)
        std::cout << "[UserDB] Cleared all users.\n";
    return ok;
}
