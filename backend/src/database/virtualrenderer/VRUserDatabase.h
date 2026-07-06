#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../sqlite/Database.h"

// ---------------------------------------
// Class: VRUserDatabase
// ---------------------------------------
// Provides interface for managing users
// using a separate users.sqlite3 database.
// ---------------------------------------
class VRUserDatabase {
public:
    // Constructor takes a reference to the Database instance
    explicit VRUserDatabase(Database& db);

    bool ensureTable();

    // Create a new user
    int createUser(const std::string& username, std::vector<uint8_t> data); // return user id
    std::vector<uint8_t> getUserProtobuf(int user_id);
    std::vector<uint8_t> getUserProtobufByUsername(const std::string& username);
    bool updateUserProtobuf(int user_id, const std::vector<uint8_t>& protobufData);
    bool updateUsername(int user_id, const std::string& newUsername);
    bool updateUserPassword(int user_id, const std::string& newPasswordHash);
     bool updateGoogleSub(int user_id, const std::string& google_sub, const std::string& email);
     bool deleteUser(int user_id);
    bool userExists(int user_id);
    std::string getUsername(int id);
    int getUserId(const std::string& username);

private:
    Database& db_;
};
