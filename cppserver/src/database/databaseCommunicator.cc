#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include "sqlite3.h"

sqlite3* db = nullptr;

// ---------------------------
// Generic Database Functions
// ---------------------------

bool openDB(const std::string& filename) {
    if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
        std::cerr << "[DB] Could not open: " << sqlite3_errmsg(db) << "\n";
        return false;
    }
    return true;
}

void closeDB() {
    if (db) sqlite3_close(db);
    db = nullptr;
}

// Execute a raw SQL command (no return)
bool execSQL(const std::string& sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "[DB] SQL error: " << errMsg << "\n";
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

// ---------------------------
// Table Management
// ---------------------------

bool createTable(const std::string& tableName, const std::map<std::string, std::string>& columns) {
    std::ostringstream oss;
    oss << "CREATE TABLE IF NOT EXISTS " << tableName << " (ID INTEGER PRIMARY KEY AUTOINCREMENT";
    for (const auto& [col, type] : columns) {
        oss << ", " << col << " " << type;
    }
    oss << ");";
    return execSQL(oss.str());
}

// ---------------------------
// Insert Rows
// ---------------------------

int insertRow(const std::string& tableName, const std::vector<std::string>& columns, const std::vector<std::string>& values) {
    if (columns.size() != values.size()) {
        std::cerr << "[DB] Insert failed: mismatched columns and values.\n";
        return -1;
    }

    std::ostringstream sql;
    sql << "INSERT INTO " << tableName << " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        sql << columns[i];
        if (i < columns.size() - 1) sql << ", ";
    }
    sql << ") VALUES (";
    for (size_t i = 0; i < values.size(); ++i) {
        sql << "?";
        if (i < values.size() - 1) sql << ", ";
    }
    sql << ");";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DB] Prepare failed: " << sqlite3_errmsg(db) << "\n";
        return -1;
    }

    for (size_t i = 0; i < values.size(); ++i)
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), values[i].c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "[DB] Insert failed: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return -1;
    }

    int rowid = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    return rowid;
}

// ---------------------------
// Read Rows
// ---------------------------

std::vector<std::map<std::string, std::string>> readRows(
    const std::string& tableName,
    const std::string& whereClause = "")
{
    std::vector<std::map<std::string, std::string>> results;
    std::ostringstream sql;
    sql << "SELECT * FROM " << tableName;
    if (!whereClause.empty()) sql << " WHERE " << whereClause;
    sql << ";";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DB] Prepare failed: " << sqlite3_errmsg(db) << "\n";
        return results;
    }

    int cols = sqlite3_column_count(stmt);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        std::map<std::string, std::string> row;
        for (int i = 0; i < cols; ++i) {
            std::string colName = sqlite3_column_name(stmt, i);
            const unsigned char* val = sqlite3_column_text(stmt, i);
            row[colName] = val ? reinterpret_cast<const char*>(val) : "";
        }
        results.push_back(row);
    }
    sqlite3_finalize(stmt);
    return results;
}

// ---------------------------
// Delete Rows
// ---------------------------

bool deleteRows(const std::string& tableName, const std::string& whereClause = "") {
    std::ostringstream sql;
    sql << "DELETE FROM " << tableName;
    if (!whereClause.empty()) sql << " WHERE " << whereClause;
    sql << ";";
    return execSQL(sql.str());
}

// ---------------------------
// Example Wrappers (Debate)
// ---------------------------

// void initDebateDB(const std::string& filename) {
//     openDB(filename);
//     createTable("DEBATE", {
//         {"USER", "TEXT NOT NULL"},
//         {"TOPIC", "TEXT NOT NULL"}
//     });
//     closeDB();
// }

// int addDebate(const std::string& user, const std::string& topic) {
//     openDB("debates.sqlite3");
//     int id = insertRow("DEBATE", {"USER", "TOPIC"}, {user, topic});
//     closeDB();
//     return id;
// }

// std::vector<std::map<std::string, std::string>> getDebates(const std::string& user) {
//     openDB("debates.sqlite3");
//     auto results = readRows("DEBATE", "USER = '" + user + "'");
//     closeDB();
//     return results;
// }

// bool removeDebate(const std::string& topic, const std::string& user) {
//     openDB("debates.sqlite3");
//     bool ok = deleteRows("DEBATE", "TOPIC = '" + topic + "' AND USER = '" + user + "'");
//     closeDB();
//     return ok;
// }
