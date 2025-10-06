#ifndef DATABASECOMMUNICATOR_H
#define DATABASECOMMUNICATOR_H

#include <string>
#include <vector>
#include <map>

// SQLite forward declaration
struct sqlite3;

// ---------------------------
// Core Database Management
// ---------------------------

// Open and close a database
bool openDB(const std::string& filename);
void closeDB();

// Execute a raw SQL command (no return)
bool execSQL(const std::string& sql);

// ---------------------------
// Table Management
// ---------------------------

// Create a table with dynamic columns: {columnName -> typeString}
bool createTable(const std::string& tableName,
                 const std::map<std::string, std::string>& columns);

// ---------------------------
// Generic CRUD Operations
// ---------------------------

// Insert a row into a table; returns new row ID or -1 on error
int insertRow(const std::string& tableName,
              const std::vector<std::string>& columns,
              const std::vector<std::string>& values);

// Read rows; returns vector of {columnName -> value} maps
std::vector<std::map<std::string, std::string>> readRows(
    const std::string& tableName,
    const std::string& whereClause = "");

// Delete rows matching a condition
bool deleteRows(const std::string& tableName,
                const std::string& whereClause = "");

// ---------------------------
// Example Wrappers (Debate-specific)
// ---------------------------

// Initialize debate database and create table if missing
void initDebateDB(const std::string& filename);

// Add a new debate for a user
int addDebate(const std::string& user, const std::string& topic);

// Retrieve all debates for a user
std::vector<std::map<std::string, std::string>> getDebates(const std::string& user);

// Remove a debate by topic and user
bool removeDebate(const std::string& topic, const std::string& user);

#endif // DATABASECOMMUNICATOR_H
