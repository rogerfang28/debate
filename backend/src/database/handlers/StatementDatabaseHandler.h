#pragma once

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include "../sqlite/databaseCommunicator.h"

// ---------------------------------------
// Class: StatementDatabaseHandler
// ---------------------------------------
// Provides a clean object-oriented interface
// for managing statements in the database.
// ---------------------------------------
class StatementDatabaseHandler {
public:
    // Constructor automatically ensures the table exists
    StatementDatabaseHandler(const std::string& dbFile = "debates.sqlite3");

    // Add a statement with text and protobuf data
    int addStatement(const std::string& rootId, const std::string& text, 
                     const std::vector<uint8_t>& protobufData);

    // Retrieve protobuf binary data for a specific statement by ID
    std::vector<uint8_t> getStatementProtobuf(const std::string& id);

    // Update protobuf data for an existing statement by ID
    bool updateStatementProtobuf(const std::string& id, const std::vector<uint8_t>& protobufData);

    // Get statement text by ID
    std::string getStatementText(const std::string& id);

    // Remove a specific statement by ID
    bool removeStatement(const std::string& id);

    // Check if a statement exists
    bool statementExists(const std::string& id);

private:
    std::string dbFilename;
    void ensureTable();  // create table if not exists
};