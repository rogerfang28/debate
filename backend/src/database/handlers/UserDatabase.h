#pragma once

#include <string>
#include <vector>
#include <optional>
#include "../sqlite/Database.h"

// ---------------------------------------
// Class: UserDatabase
// ---------------------------------------
// Provides interface for managing users
// using the Database wrapper class.
// ---------------------------------------
class UserDatabase {
public:
    // Constructor takes a reference to the Database instance
    explicit UserDatabase(Database& db);

    bool ensureTable();

    // Create a new user
    int createUser(const std::string& username, std::vector<uint8_t> data); // return user id
    std::vector<uint8_t> getUserProtobuf(int user_id);
    std::vector<uint8_t> getUserProtobufByUsername(const std::string& username);
    bool updateUserProtobuf(const std::string& user_id, const std::vector<uint8_t>& protobufData);
    bool updateUsername(const std::string& user_id, const std::string& newUsername);
    bool updateUserPassword(const std::string& user_id, const std::string& newPasswordHash);    
    bool deleteUser(const std::string& user_id);
    bool userExists(const std::string& user_id);
    std::string getUsername(int id);
    int getUserId(const std::string& username);

private:
    Database& db_;
};
