#include "UserDatabaseHandler.h"
#include <iostream>
#include <vector>
#include <cstdint>  // for std::vector<uint8_t>
#include "../database/databaseCommunicator.h"

UserDatabaseHandler::UserDatabaseHandler(const std::string& dbFile)
    : dbFilename(dbFile) {
    ensureTable();
}

// ---------------------------
// Create Table
// ---------------------------

void UserDatabaseHandler::ensureTable() {
    if (!openDB(dbFilename)) return;

    createTable("USERS", {
        {"USER", "TEXT NOT NULL UNIQUE"},
        {"NAME", "TEXT"},
        {"JOINDATE", "TEXT"},
        {"LOCATION", "TEXT"},
        {"PROFILE_DATA", "BLOB"}  // optional serialized protobuf or binary user data
    });

    closeDB();
}

// ---------------------------
// Add Users
// ---------------------------

int UserDatabaseHandler::addUser(const std::string& username,
                                 const std::string& name,
                                 const std::string& joinDate,
                                 const std::string& location) {
    if (!openDB(dbFilename)) return -1;

    int id = insertRowWithText(
        "USERS",
        {"USER", "NAME", "JOINDATE", "LOCATION"},
        {username, name, joinDate, location}
    );

    closeDB();

    if (id != -1)
        std::cout << "[UserDB] Added user: " << username
                  << " (" << name << ", " << location << ")\n";

    return id;
}

int UserDatabaseHandler::addUserWithProtobuf(const std::string& username,
                                             const std::string& name,
                                             const std::string& joinDate,
                                             const std::string& location,
                                             const std::vector<uint8_t>& protobufData) {
    if (!openDB(dbFilename)) return -1;

    std::vector<std::pair<const void*, int>> blobValues;
    if (!protobufData.empty())
        blobValues.push_back({protobufData.data(), static_cast<int>(protobufData.size())});
    else
        blobValues.push_back({nullptr, 0});

    int id = insertRowWithBlob(
        "USERS",
        {"USER", "NAME", "JOINDATE", "LOCATION", "PROFILE_DATA"},
        {username, name, joinDate, location},
        blobValues
    );

    closeDB();

    if (id != -1)
        std::cout << "[UserDB] Added user with protobuf: " << username
                  << " (" << name << ", " << location
                  << ") [data size: " << protobufData.size() << " bytes]\n";

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

std::vector<std::map<std::string, std::string>>
UserDatabaseHandler::getUser(const std::string& username) {
    if (!openDB(dbFilename)) return {};
    auto rows = readRows("USERS", "USER = '" + username + "'");
    closeDB();

    std::cout << "[UserDB] Retrieved " << rows.size()
              << " record(s) for user " << username << "\n";
    return rows;
}

std::vector<uint8_t>
UserDatabaseHandler::getUserProtobuf(const std::string& username) {
    if (!openDB(dbFilename)) return {};
    auto blob = readBlob("USERS", "PROFILE_DATA", "USER = '" + username + "'");
    closeDB();

    std::cout << "[UserDB] Retrieved protobuf data for user " << username
              << " (size: " << blob.size() << " bytes)\n";
    return blob;
}

// ---------------------------
// Update / Delete
// ---------------------------

bool UserDatabaseHandler::updateUserProtobuf(const std::string& username,
                                             const std::vector<uint8_t>& protobufData) {
    std::cout << "[UserDB] Updating protobuf data for user " << username
              << " (size: " << protobufData.size() << " bytes)\n";

    if (!openDB(dbFilename)) return false;
    bool ok = updateRowWithBlob("USERS", "PROFILE_DATA", protobufData,
                                "USER = '" + username + "'");
    closeDB();

    if (ok)
        std::cout << "[UserDB] Updated protobuf data for " << username << "\n";
    return ok;
}

bool UserDatabaseHandler::removeUser(const std::string& username) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("USERS", "USER = '" + username + "'");
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
