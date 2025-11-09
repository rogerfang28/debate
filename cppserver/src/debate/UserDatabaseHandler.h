#pragma once

#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <iostream>
#include "../database/databaseCommunicator.h"  // for openDB, insertRowWithText, etc.

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
    int addUser(const std::string& username,
                const std::string& name,
                const std::string& joinDate,
                const std::string& location);

    int addUserWithProtobuf(const std::string& username,
                            const std::string& name,
                            const std::string& joinDate,
                            const std::string& location,
                            const std::vector<uint8_t>& protobufData);

    // ---------------------------
    // Read / Get
    // ---------------------------
    std::vector<std::map<std::string, std::string>> getAllUsers();
    std::vector<std::map<std::string, std::string>> getUser(const std::string& username);
    std::vector<uint8_t> getUserProtobuf(const std::string& username);

    // ---------------------------
    // Update / Delete
    // ---------------------------
    bool updateUserLocation(const std::string& username, const std::string& newLocation);
    
    bool updateUserProtobuf(const std::string& username,
                            const std::vector<uint8_t>& protobufData);

    bool removeUser(const std::string& username);
    bool clearAllUsers();

private:
    std::string dbFilename;
};
