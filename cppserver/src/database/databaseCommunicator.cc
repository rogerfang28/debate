#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include "sqlite3.h"
#include <cstdint>  // for std::vector<uint8_t>


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
    if (db) {
        sqlite3_close(db);
        db = nullptr;
    }
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

int insertRowWithText(const std::string& tableName, const std::vector<std::string>& columns, const std::vector<std::string>& values) {
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

int insertRowWithBlob(
    const std::string& tableName,
    const std::vector<std::string>& columns,
    const std::vector<std::string>& textValues,
    const std::vector<std::pair<const void*, int>>& blobValues)
{
    if (columns.size() != textValues.size() + blobValues.size()) {
        std::cerr << "[DB] Insert failed: mismatched column count.\n";
        return -1;
    }

    std::ostringstream sql;
    sql << "INSERT INTO " << tableName << " (";
    for (size_t i = 0; i < columns.size(); ++i) {
        sql << columns[i];
        if (i < columns.size() - 1) sql << ", ";
    }
    sql << ") VALUES (";
    for (size_t i = 0; i < columns.size(); ++i) {
        sql << "?";
        if (i < columns.size() - 1) sql << ", ";
    }
    sql << ");";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DB] Prepare failed: " << sqlite3_errmsg(db) << "\n";
        return -1;
    }

    int paramIndex = 1;

    // Bind text values first
    for (const auto& v : textValues)
        sqlite3_bind_text(stmt, paramIndex++, v.c_str(), -1, SQLITE_TRANSIENT);

    // Then bind blob values
    for (const auto& [ptr, size] : blobValues)
        sqlite3_bind_blob(stmt, paramIndex++, ptr, size, SQLITE_TRANSIENT);

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

std::vector<uint8_t> readBlob(
    const std::string& tableName,
    const std::string& blobColumn,
    const std::string& whereClause)
{
    std::vector<uint8_t> data;
    std::ostringstream sql;
    sql << "SELECT " << blobColumn << " FROM " << tableName;
    if (!whereClause.empty()) sql << " WHERE " << whereClause;
    sql << " LIMIT 1;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DB] Prepare failed: " << sqlite3_errmsg(db) << "\n";
        return data;
    }

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int bytes = sqlite3_column_bytes(stmt, 0);
        if (blob && bytes > 0)
            data.assign((const uint8_t*)blob, (const uint8_t*)blob + bytes);
    }

    sqlite3_finalize(stmt);
    return data;
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
// Update Rows
// ---------------------------

bool updateRowWithBlob(const std::string& tableName, const std::string& blobColumn,
                       const std::vector<uint8_t>& blobData, const std::string& whereClause) {
    if (whereClause.empty()) {
        std::cerr << "[DB] Update failed: WHERE clause is required for safety.\n";
        return false;
    }

    std::ostringstream sql;
    sql << "UPDATE " << tableName << " SET " << blobColumn << " = ? WHERE " << whereClause << ";";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[DB] Prepare failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    if (!blobData.empty()) {
        sqlite3_bind_blob(stmt, 1, blobData.data(), static_cast<int>(blobData.size()), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 1);
    }

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    if (rc != SQLITE_DONE) {
        std::cerr << "[DB] Update failed: " << sqlite3_errmsg(db) << "\n";
        return false;
    }

    return true;
}