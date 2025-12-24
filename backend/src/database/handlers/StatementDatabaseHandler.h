#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <iostream>

// ---------------------------------------
// Class: StatementDatabaseHandler
// ---------------------------------------
// Provides a clean object-oriented interface
// for managing statements in the database.
// ---------------------------------------
class StatementDatabaseHandler {
public:
    // ---------------------------
    // Constructor / Destructor
    // ---------------------------
    explicit StatementDatabaseHandler(const std::string& dbFile);
    ~StatementDatabaseHandler() = default;

    // ---------------------------
    // Table Setup
    // ---------------------------
    void ensureTable();

    // ---------------------------
    // Create / Add
    // ---------------------------
    int addStatement(const std::string& rootId, const std::string& text, 
                     const std::vector<uint8_t>& protobufData);

    // ---------------------------
    // Read / Get
    // ---------------------------
    std::vector<uint8_t> getStatementProtobuf(const std::string& id);
    std::string getStatementText(const std::string& id);
    std::string getStatementRootId(const std::string& id);
    bool statementExists(const std::string& id);

    // ---------------------------
    // Update / Delete
    // ---------------------------
    bool updateStatementProtobuf(const std::string& id, const std::vector<uint8_t>& protobufData);
    bool removeStatement(const std::string& id);

private:
    std::string dbFilename;
};