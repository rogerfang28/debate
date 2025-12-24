#include "StatementDatabaseHandler.h"

StatementDatabaseHandler::StatementDatabaseHandler(const std::string& dbFile)
    : dbFilename(dbFile) {
    ensureTable();
}

void StatementDatabaseHandler::ensureTable() {
    if (!openDB(dbFilename)) return;
    createTable("STATEMENTS", {
        {"ROOT_ID", "TEXT NOT NULL"},
        {"TEXT", "TEXT NOT NULL"},
        {"BLOB", "BLOB"} // binary protobuf
    });
    closeDB();
}

int StatementDatabaseHandler::addStatement(const std::string& rootId, const std::string& text, 
                                         const std::vector<uint8_t>& protobufData) {
    if (!openDB(dbFilename)) return -1;
    
    std::vector<std::pair<const void*, int>> blobValues;
    if (!protobufData.empty()) {
        blobValues.push_back({protobufData.data(), static_cast<int>(protobufData.size())});
    } else {
        blobValues.push_back({nullptr, 0});
    }
    
    int id = insertRowWithBlob("STATEMENTS", {"ROOT_ID", "TEXT", "BLOB"}, 
                               {rootId, text}, blobValues);
    closeDB();
    if (id != -1)
        std::cout << "[StatementDB] Added statement: " << text 
                  << " with root " << rootId
                  << " (data size: " << protobufData.size() << " bytes)\n";
    return id;
}

std::vector<uint8_t> StatementDatabaseHandler::getStatementProtobuf(const std::string& id) {
    if (!openDB(dbFilename)) return {};
    
    std::string whereClause = "ID = '" + id + "'";
    auto protobufData = readBlob("STATEMENTS", "BLOB", whereClause);
    closeDB();
    
    std::cout << "[StatementDB] Retrieved protobuf data for id: " << id 
              << " (size: " << protobufData.size() << " bytes)\n";
    return protobufData;
}

bool StatementDatabaseHandler::updateStatementProtobuf(const std::string& id, 
                                                     const std::vector<uint8_t>& protobufData) {
    std::cout << "[StatementDB] Updating protobuf data for id: " << id 
              << " (size: " << protobufData.size() << " bytes)\n";
    if (!openDB(dbFilename)) return false;
    
    std::string whereClause = "ID = '" + id + "'";
    bool success = updateRowWithBlob("STATEMENTS", "BLOB", protobufData, whereClause);
    closeDB();
    
    if (success) {
        std::cout << "[StatementDB] Updated protobuf data for id: " << id 
                  << " (size: " << protobufData.size() << " bytes)\n";
    }
    return success;
}

std::string StatementDatabaseHandler::getStatementText(const std::string& id) {
    if (!openDB(dbFilename)) return "";
    
    auto rows = readRows("STATEMENTS", "ID = '" + id + "'");
    closeDB();
    
    if (!rows.empty() && rows[0].count("TEXT")) {
        std::cout << "[StatementDB] Retrieved text for id: " << id << "\n";
        return rows[0]["TEXT"];
    }
    return "";
}

bool StatementDatabaseHandler::removeStatement(const std::string& id) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("STATEMENTS", "ID = '" + id + "'");
    closeDB();
    if (ok)
        std::cout << "[StatementDB] Removed statement with id: " << id << "\n";
    return ok;
}

bool StatementDatabaseHandler::statementExists(const std::string& id) {
    if (!openDB(dbFilename)) return false;
    auto rows = readRows("STATEMENTS", "ID = '" + id + "'");
    closeDB();
    bool exists = !rows.empty();
    std::cout << "[StatementDB] Statement with id: " << id 
              << (exists ? " exists.\n" : " does not exist.\n");
    return exists;
}