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

    int addStatement(int root_id, const std::string& content, std::vector<uint8_t> protobufData, int creatorId = -1); // return statement id
    std::vector<uint8_t> getStatementProtobuf(int statementId);
    int getStatementCreatorId(int statementId);

    bool updateStatementContent(int statementId, const std::string& newContent);
    bool updateStatementProtobuf(int statementId, const std::vector<uint8_t>& protobufData);
    bool updateStatementRoot(int statementId, int newRootId);
    bool updateStatementCreatorId(int statementId, int creatorId);
    bool deleteStatement(int statementId);
    bool statementExists(int statementId);

private:
    Database& db_;
};
