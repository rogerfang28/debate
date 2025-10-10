#ifndef DATABASE_COMMUNICATOR_H
#define DATABASE_COMMUNICATOR_H

#include <string>
#include <vector>
#include <map>
#include <cstdint>  // for std::vector<uint8_t>

// Forward declare sqlite3 so we don't need to include <sqlite3.h> in every file
struct sqlite3;
extern sqlite3* db;

// ---------------------------
// Generic Database Functions
// ---------------------------
bool openDB(const std::string& filename);
void closeDB();
bool execSQL(const std::string& sql);

// ---------------------------
// Table Management
// ---------------------------
bool createTable(const std::string& tableName,
                 const std::map<std::string, std::string>& columns);

// ---------------------------
// Insert Rows
// ---------------------------
int insertRowWithText(const std::string& tableName,
                      const std::vector<std::string>& columns,
                      const std::vector<std::string>& values);

int insertRowWithBlob(const std::string& tableName,
                      const std::vector<std::string>& columns,
                      const std::vector<std::string>& textValues,
                      const std::vector<std::pair<const void*, int>>& blobValues);

// ---------------------------
// Read Rows
// ---------------------------
std::vector<std::map<std::string, std::string>> readRows(
    const std::string& tableName,
    const std::string& whereClause = "");

std::vector<uint8_t> readBlob(const std::string& tableName,
                              const std::string& blobColumn,
                              const std::string& whereClause);

// ---------------------------
// Delete Rows
// ---------------------------
bool deleteRows(const std::string& tableName,
                const std::string& whereClause = "");

// ---------------------------
// Update Rows
// ---------------------------
bool updateRowWithBlob(const std::string& tableName, const std::string& blobColumn,
                       const std::vector<uint8_t>& blobData, const std::string& whereClause);

#endif // DATABASE_COMMUNICATOR_H
