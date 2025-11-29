#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <iostream>
#include "../sqlite/databaseCommunicator.h"  // for openDB, insertRowWithText, etc.

class UserDatabaseHandler {
public:
    // ---------------------------
    // Constructor / Destructor
    // ---------------------------
    explicit UserDatabaseHandler(const std::string& dbFile);
    ~UserDatabaseHandler() = default;

    // ---------------------------
    // Table Setup
    // ---------------------------
    void ensureTable();

    // ---------------------------
    // Create / Add
    // ---------------------------
    int addUser(const std::string& username, const std::vector<uint8_t>& protobufData);

    // ---------------------------
    // Read / Get
    // ---------------------------
    std::vector<std::map<std::string, std::string>> getAllUsers();
    std::vector<uint8_t> getUserProtobuf(const std::string& username);
    bool userExists(const std::string& username);

    // ---------------------------
    // Update / Delete
    // ---------------------------
    bool updateUserProtobuf(const std::string& username, const std::vector<uint8_t>& protobufData);
    bool removeUser(const std::string& username);
    bool clearAllUsers();

private:
    std::string dbFilename;
};
