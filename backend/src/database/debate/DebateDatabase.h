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

    int addDebate(int userId, const std::string& topic, const std::vector<uint8_t>& protobufData); // return id

    // std::vector<std::map<std::string, std::string>> getDebates(int userId); // not really needed
    std::vector<uint8_t> getDebateProtobuf(int debateId);
    bool updateDebateProtobuf(int debateId, int userId, const std::vector<uint8_t>& protobufData);
    bool removeDebate(int debateId, int userId);
    bool clearUserDebates(int userId);
    bool debateExists(int debateId);

private:
    Database& db_;
};
