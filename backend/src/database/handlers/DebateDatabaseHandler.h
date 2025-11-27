#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include "../sqlite/DatabaseCommunicator.h"

// ---------------------------------------
// Class: DebateDatabaseHandler
// ---------------------------------------
// Provides a clean object-oriented interface
// for managing debate topics in the database.
// ---------------------------------------
class DebateDatabaseHandler {
public:
    // Constructor automatically ensures the table exists
    DebateDatabaseHandler(const std::string& dbFile = "debates.sqlite3");

    // Add a debate for a given user
    int addDebate(const std::string& user, const std::string& topic);

    // Add a debate with protobuf binary data for a given user
    int addDebateWithProtobuf(const std::string& user, const std::string& topic, 
                              const std::vector<uint8_t>& protobufData);

    // Retrieve all debates for a specific user
    std::vector<std::map<std::string, std::string>> getDebates(const std::string& user);

    // Retrieve protobuf binary data for a specific debate by ID
    std::vector<uint8_t> getDebateProtobuf(const std::string& user, const std::string& id);

    // Update protobuf data for an existing debate by ID
    bool updateDebateProtobuf(const std::string& user, const std::string& id, 
                              const std::vector<uint8_t>& protobufData);

    // Remove a specific debate for a user by ID
    bool removeDebate(const std::string& id, const std::string& user);

    // Clear all debates for a given user
    bool clearUserDebates(const std::string& user);

private:
    std::string dbFilename;
    void ensureTable();  // create table if not exists
};
