#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <mutex>
#include "sqlite3.h"
#include <cstdint>  // for std::vector<uint8_t>
#include "../../utils/Log.h"


sqlite3* db = nullptr;
std::mutex db_mutex;  // Protect all database operations

// ---------------------------
// Generic Database Functions
// ---------------------------

bool openDB(const std::string& filename) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    // Close existing connection if any
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
    }
    
    // Use thread-safe mode
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(filename.c_str(), &db, flags, nullptr) != SQLITE_OK) {
        Log::error("[DB] Could not open: " + std::string(sqlite3_errmsg(db)));
        return false;
    }
    return true;
}

void closeDB() {
    std::lock_guard<std::mutex> lock(db_mutex);
    if (db != nullptr) {
        sqlite3_close(db);
        db = nullptr;
    }
}

// Execute a raw SQL command (no return)
bool execSQL(const std::string& sql) {
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if (db == nullptr) {
        Log::error("[DB] Error: Database not open!");
        return false;
    }
    
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        Log::error("[DB] SQL error: " + std::string(errMsg));
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
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if (db == nullptr) {
        Log::error("[DB] Error: Database not open!");
        return -1;
    }
    
    if (columns.size() != values.size()) {
        Log::error("[DB] Insert failed: mismatched columns and values.");
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
        Log::error("[DB] Prepare failed: " + std::string(sqlite3_errmsg(db)));
        return -1;
    }

    for (size_t i = 0; i < values.size(); ++i)
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), values[i].c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        Log::error("[DB] Insert failed: " + std::string(sqlite3_errmsg(db)));
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
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if (db == nullptr) {
        Log::error("[DB] Error: Database not open!");
        return -1;
    }
    
    if (columns.size() != textValues.size() + blobValues.size()) {
        Log::error("[DB] Insert failed: mismatched column count.");
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
        Log::error("[DB] Prepare failed: " + std::string(sqlite3_errmsg(db)));
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
        Log::error("[DB] Insert failed: " + std::string(sqlite3_errmsg(db)));
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
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::vector<std::map<std::string, std::string>> results;
    
    if (db == nullptr) {
        Log::error("[DB] Error: Database not open!");
        return results;
    }
    
    std::ostringstream sql;
    sql << "SELECT * FROM " << tableName;
    if (!whereClause.empty()) sql << " WHERE " << whereClause;
    sql << ";";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        Log::error("[DB] Prepare failed: " + std::string(sqlite3_errmsg(db)));
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
    std::lock_guard<std::mutex> lock(db_mutex);
    
    std::vector<uint8_t> data;
    
    if (db == nullptr) {
        Log::error("[DB] Error: Database not open!");
        return data;
    }
    
    std::ostringstream sql;
    sql << "SELECT " << blobColumn << " FROM " << tableName;
    if (!whereClause.empty()) sql << " WHERE " << whereClause;
    sql << " LIMIT 1;";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        Log::error("[DB] Prepare failed: " + std::string(sqlite3_errmsg(db)));
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
    std::lock_guard<std::mutex> lock(db_mutex);
    
    if (db == nullptr) {
        Log::error("[DB] Error: Database not open!");
        return false;
    }
    
    if (whereClause.empty()) {
        Log::error("[DB] Update failed: WHERE clause is required for safety.");
        return false;
    }

    std::ostringstream sql;
    sql << "UPDATE " << tableName << " SET " << blobColumn << " = ? WHERE " << whereClause << ";";

    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql.str().c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        Log::error("[DB] Prepare failed: " + std::string(sqlite3_errmsg(db)));
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
        Log::error("[DB] Update failed: " + std::string(sqlite3_errmsg(db)));
        return false;
    }

    return true;
}