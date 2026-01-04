#pragma once

#include <sqlite3.h>
#include <string>
#include <mutex>

class Database {
public:
    // Constructor: Opens the SQLite database at the given path.
    // Sets WAL mode and enables foreign keys.
    explicit Database(const std::string& path);

    // Destructor: Closes the SQLite connection if open.
    ~Database();

    // Non-copyable
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    // Execute SQL statements that don't return rows (CREATE, INSERT, UPDATE, DELETE, PRAGMA).
    // Returns true on success, false on error.
    bool execute(const std::string& sql);

    // Prepare a statement for execution (typically for SELECT or parameterized queries).
    // The caller is responsible for calling sqlite3_finalize(stmt) when done.
    // Returns nullptr on error.
    sqlite3_stmt* prepare(const std::string& sql);

    // Transaction helpers
    bool beginTransaction();
    bool commit();
    bool rollback();

    // Returns the raw sqlite3* handle for low-level access.
    sqlite3* handle() const;

private:
    sqlite3* db_ = nullptr;
    std::mutex mutex_;
    std::string path_;
};
