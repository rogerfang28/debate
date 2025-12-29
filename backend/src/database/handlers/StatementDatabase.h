#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include "../sqlite/Database.h"

// ---------------------------------------
// Class: StatementDatabase
// ---------------------------------------
// Provides interface for managing debate statements/claims
// using the Database wrapper class.
// ---------------------------------------
class StatementDatabase {
public:
    // Constructor takes a reference to the Database instance
    explicit StatementDatabase(Database& db);

    bool ensureTable();

    int addStatement(int root_id, const std::string& content, std::vector<uint8_t> protobufData, const std::string& creator = ""); // return statement id
    std::vector<uint8_t> getStatementProtobuf(int statementId);
    std::string getStatementCreator(int statementId);

    bool updateStatementContent(int statementId, const std::string& newContent);
    bool updateStatementProtobuf(int statementId, const std::vector<uint8_t>& protobufData);
    bool updateStatementRoot(int statementId, int newRootId);
    bool updateStatementCreator(int statementId, const std::string& creator);
    bool deleteStatement(int statementId);
    bool statementExists(int statementId);

private:
    Database& db_;
};
