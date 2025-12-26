#include "StatementDatabase.h"
#include <iostream>
#include "../../utils/Log.h"

StatementDatabase::StatementDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool StatementDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS STATEMENTS (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            ROOT_ID TEXT NOT NULL,
            TEXT TEXT NOT NULL,
            STATEMENT_DATA BLOB
        );
    )";
    return db_.execute(sql);
}

int StatementDatabase::addStatement(int root_id, const std::string& content, std::vector<uint8_t> protobufData) {
    const char* sql = "INSERT INTO STATEMENTS (ROOT_ID, TEXT, STATEMENT_DATA) VALUES (?, ?, ?);";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return -1;
    }

    sqlite3_bind_text(stmt, 1, std::to_string(root_id).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, content.c_str(), -1, SQLITE_TRANSIENT);
    
    if (!protobufData.empty()) {
        sqlite3_bind_blob(stmt, 3, protobufData.data(), protobufData.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 3);
    }

    int result = sqlite3_step(stmt);
    int statementId = -1;
    
    if (result == SQLITE_DONE) {
        statementId = static_cast<int>(sqlite3_last_insert_rowid(db_.handle()));
    } else {
        std::cerr << "Failed to insert statement: " << sqlite3_errmsg(db_.handle()) << std::endl;
    }

    sqlite3_finalize(stmt);
    return statementId;
}

std::vector<uint8_t> StatementDatabase::getStatementProtobuf(int statementId) {
    std::vector<uint8_t> protobufData;
    const char* sql = "SELECT STATEMENT_DATA FROM STATEMENTS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return protobufData;
    }

    sqlite3_bind_int(stmt, 1, statementId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int blobSize = sqlite3_column_bytes(stmt, 0);
        
        if (blob && blobSize > 0) {
            const uint8_t* data = static_cast<const uint8_t*>(blob);
            protobufData.assign(data, data + blobSize);
        }
    }

    sqlite3_finalize(stmt);
    return protobufData;
}

bool StatementDatabase::updateStatementContent(int statementId, const std::string& newContent) {
    const char* sql = "UPDATE STATEMENTS SET TEXT = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, newContent.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, statementId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool StatementDatabase::updateStatementProtobuf(int statementId, const std::vector<uint8_t>& protobufData) {
    const char* sql = "UPDATE STATEMENTS SET STATEMENT_DATA = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    if (!protobufData.empty()) {
        sqlite3_bind_blob(stmt, 1, protobufData.data(), protobufData.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_int(stmt, 2, statementId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool StatementDatabase::updateStatementRoot(int statementId, int newRootId) {
    const char* sql = "UPDATE STATEMENTS SET ROOT_ID = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, std::to_string(newRootId).c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, statementId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool StatementDatabase::deleteStatement(int statementId) {
    const char* sql = "DELETE FROM STATEMENTS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, statementId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool StatementDatabase::statementExists(int statementId) {
    const char* sql = "SELECT 1 FROM STATEMENTS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, statementId);

    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);

    sqlite3_finalize(stmt);
    return exists;
}
