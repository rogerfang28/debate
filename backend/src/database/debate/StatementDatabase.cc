#include "StatementDatabase.h"
#include <iostream>
#include "../../utils/Log.h"

#include <sstream>

namespace {
bool statementTableHasColumn(Database& db, const char* columnName) {
    const char* sql = "PRAGMA table_info(STATEMENTS);";
    sqlite3_stmt* stmt = db.prepare(sql);
    if (!stmt) {
        return false;
    }

    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (name != nullptr && std::string(name) == columnName) {
            found = true;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}
}

StatementDatabase::StatementDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool StatementDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS STATEMENTS (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            ROOT_ID TEXT NOT NULL,
            TEXT TEXT NOT NULL,
            STATEMENT_DATA BLOB,
            CREATOR_ID INTEGER,
            DEBATE_ID INTEGER
        );
    )";
    if (!db_.execute(sql)) {
        return false;
    }

    if (!statementTableHasColumn(db_, "DEBATE_ID")) {
        if (!db_.execute("ALTER TABLE STATEMENTS ADD COLUMN DEBATE_ID INTEGER;")) {
            return false;
        }
    }

    return true;
}

int StatementDatabase::addStatement(int root_id, const std::string& content, std::vector<uint8_t> protobufData, int creatorId, int debateId) {
    const char* sql = "INSERT INTO STATEMENTS (ROOT_ID, TEXT, STATEMENT_DATA, CREATOR_ID, DEBATE_ID) VALUES (?, ?, ?, ?, ?);";
    
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

    if (creatorId != -1) {
        sqlite3_bind_int(stmt, 4, creatorId);
    } else {
        sqlite3_bind_null(stmt, 4);
    }

    if (debateId != -1) {
        sqlite3_bind_int(stmt, 5, debateId);
    } else {
        sqlite3_bind_null(stmt, 5);
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

std::vector<std::vector<uint8_t>> StatementDatabase::getStatementsForDebateAndCreators(int debateId, const std::vector<int>& creatorIds) {
    std::vector<std::vector<uint8_t>> statements;
    if (creatorIds.empty()) {
        return statements;
    }

    std::ostringstream sql;
    sql << "SELECT STATEMENT_DATA FROM STATEMENTS WHERE DEBATE_ID = ? AND CREATOR_ID IN (";
    for (size_t i = 0; i < creatorIds.size(); ++i) {
        if (i > 0) {
            sql << ", ";
        }
        sql << "?";
    }
    sql << ");";

    sqlite3_stmt* stmt = db_.prepare(sql.str());
    if (!stmt) {
        return statements;
    }

    sqlite3_bind_int(stmt, 1, debateId);
    for (size_t i = 0; i < creatorIds.size(); ++i) {
        sqlite3_bind_int(stmt, static_cast<int>(i + 2), creatorIds[i]);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int blobSize = sqlite3_column_bytes(stmt, 0);
        if (blob != nullptr && blobSize > 0) {
            const uint8_t* data = static_cast<const uint8_t*>(blob);
            statements.emplace_back(data, data + blobSize);
        }
    }

    sqlite3_finalize(stmt);
    return statements;
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

int StatementDatabase::getStatementCreatorId(int statementId) {
    int creatorId = -1;
    const char* sql = "SELECT CREATOR_ID FROM STATEMENTS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return creatorId;
    }

    sqlite3_bind_int(stmt, 1, statementId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            creatorId = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    return creatorId;
}

int StatementDatabase::getStatementDebateId(int statementId) {
    int debateId = -1;
    const char* sql = "SELECT DEBATE_ID FROM STATEMENTS WHERE ID = ?;";

    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return debateId;
    }

    sqlite3_bind_int(stmt, 1, statementId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            debateId = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    return debateId;
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

bool StatementDatabase::updateStatementDebateId(int statementId, int debateId) {
    const char* sql = "UPDATE STATEMENTS SET DEBATE_ID = ? WHERE ID = ?;";

    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, debateId);
    sqlite3_bind_int(stmt, 2, statementId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool StatementDatabase::updateStatementCreatorId(int statementId, int creatorId) {
    const char* sql = "UPDATE STATEMENTS SET CREATOR_ID = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    if (creatorId != -1) {
        sqlite3_bind_int(stmt, 1, creatorId);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
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
