#include "StatementDatabaseHandler.h"
#include <iostream>
#include <vector>
#include <cstdint>
#include "../sqlite/databaseCommunicator.h"
#include "../../utils/Log.h"

StatementDatabaseHandler::StatementDatabaseHandler(const std::string& dbFile)
    : dbFilename(dbFile) {
    ensureTable();
}

// ---------------------------
// Create Table
// ---------------------------

void StatementDatabaseHandler::ensureTable() {
    if (!openDB(dbFilename)) return;

    createTable("STATEMENTS", {
        {"ROOT_ID", "TEXT NOT NULL"},
        {"TEXT", "TEXT NOT NULL"},
        {"STATEMENT_DATA", "BLOB"}  // serialized statement protobuf
    });

    closeDB();
}

// ---------------------------
// Add Statement
// ---------------------------

int StatementDatabaseHandler::addStatement(const std::string& rootId, const std::string& text, 
                                         const std::vector<uint8_t>& protobufData) {
    if (!openDB(dbFilename)) return -1;
    
    std::vector<std::pair<const void*, int>> blobValues;
    if (!protobufData.empty())
        blobValues.push_back({protobufData.data(), static_cast<int>(protobufData.size())});
    else
        blobValues.push_back({nullptr, 0});
    
    int id = insertRowWithBlob(
        "STATEMENTS", 
        {"ROOT_ID", "TEXT", "STATEMENT_DATA"}, 
        {rootId, text}, 
        blobValues
    );
    
    closeDB();
    
    if (id != -1)
        Log::debug("[StatementDB] Added statement: " + text 
                  + " with root " + rootId
                  + " [data size: " + std::to_string(protobufData.size()) + " bytes]");
    
    return id;
}

// ---------------------------
// Retrieve Statements
// ---------------------------

std::vector<uint8_t> StatementDatabaseHandler::getStatementProtobuf(const std::string& id) {
    if (!openDB(dbFilename)) return {};
    
    auto blob = readBlob("STATEMENTS", "STATEMENT_DATA", "ID = '" + id + "'");
    closeDB();
    
    // std::cout << "[StatementDB] Retrieved protobuf data for id: " << id 
    //           << " (size: " << blob.size() << " bytes)\n";
    return blob;
}

std::string StatementDatabaseHandler::getStatementText(const std::string& id) {
    if (!openDB(dbFilename)) return "";
    
    auto rows = readRows("STATEMENTS", "ID = '" + id + "'");
    closeDB();
    
    if (!rows.empty() && rows[0].count("TEXT")) {
        Log::debug("[StatementDB] Retrieved text for id: " + id);
        return rows[0]["TEXT"];
    }
    return "";
}

std::string StatementDatabaseHandler::getStatementRootId(const std::string& id) {
    if (!openDB(dbFilename)) return "";
    
    auto rows = readRows("STATEMENTS", "ID = '" + id + "'");
    closeDB();
    
    if (!rows.empty() && rows[0].count("ROOT_ID")) {
        Log::debug("[StatementDB] Retrieved root ID for id: " + id);
        return rows[0]["ROOT_ID"];
    }
    return "";
}

bool StatementDatabaseHandler::statementExists(const std::string& id) {
    if (!openDB(dbFilename)) return false;
    auto rows = readRows("STATEMENTS", "ID = '" + id + "'");
    closeDB();
    
    bool exists = !rows.empty();
    Log::debug("[StatementDB] Statement with id: " + id 
              + (exists ? " exists." : " does not exist."));
    return exists;
}

// ---------------------------
// Update / Delete
// ---------------------------

bool StatementDatabaseHandler::updateStatementProtobuf(const std::string& id, 
                                                     const std::vector<uint8_t>& protobufData) {
    Log::debug("[StatementDB] Updating protobuf data for id: " + id 
              + " (size: " + std::to_string(protobufData.size()) + " bytes)");
    
    if (!openDB(dbFilename)) return false;
    bool ok = updateRowWithBlob("STATEMENTS", "STATEMENT_DATA", protobufData,
                                "ID = '" + id + "'");
    closeDB();
    
    if (ok)
        Log::debug("[StatementDB] Updated protobuf data for id: " + id);
    return ok;
}

bool StatementDatabaseHandler::removeStatement(const std::string& id) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("STATEMENTS", "ID = '" + id + "'");
    closeDB();
    
    if (ok)
        Log::debug("[StatementDB] Removed statement with id: " + id);
    return ok;
}