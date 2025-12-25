#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include "../sqlite/Database.h"

// ---------------------------------------
// Class: DebateDatabase
// ---------------------------------------
// Provides interface for managing debate topics
// using the Database wrapper class.
// ---------------------------------------
class DebateDatabase {
public:
    // Constructor takes a reference to the Database instance
    explicit DebateDatabase(Database& db);

    bool ensureTable();

    int addDebate(const std::string& user, const std::string& topic, const std::vector<uint8_t>& protobufData); // return id

    // std::vector<std::map<std::string, std::string>> getDebates(const std::string& user); // not really needed
    std::vector<uint8_t> getDebateProtobuf(int debateId);
    bool updateDebateProtobuf(int debateId, const std::string& user, const std::vector<uint8_t>& protobufData);
    bool removeDebate(int debateId, const std::string& user);
    bool clearUserDebates(const std::string& user);
    bool debateExists(int debateId);

private:
    Database& db_;
};
