#include "Database.h"
#include "../../utils/Log.h"
#include <iostream>

Database::Database(const std::string& path) : path_(path) {
    int result = sqlite3_open(path.c_str(), &db_);
    if (result != SQLITE_OK) {
        Log::error("Failed to open database: " + std::string(sqlite3_errmsg(db_)));
        if (db_) {
            sqlite3_close(db_);
            db_ = nullptr;
        }
        return;
    }

    // Enable WAL mode
    if (!execute("PRAGMA journal_mode=WAL;")) {
        Log::error("Failed to enable WAL mode");
    }

    // Enable foreign keys
    if (!execute("PRAGMA foreign_keys=ON;")) {
        Log::error("Failed to enable foreign keys");
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::execute(const std::string& sql) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_) {
        std::cerr << "Database not open" << std::endl;
        return false;
    }

    char* errorMsg = nullptr;
    int result = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errorMsg);
    
    if (result != SQLITE_OK) {
        Log::error("SQL error: " + std::string(errorMsg ? errorMsg : "unknown error"));
        if (errorMsg) {
            sqlite3_free(errorMsg);
        }
        return false;
    }

    return true;
}

sqlite3_stmt* Database::prepare(const std::string& sql) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!db_) {
        std::cerr << "Database not open" << std::endl;
        return nullptr;
    }

    sqlite3_stmt* stmt = nullptr;
    int result = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
    
    if (result != SQLITE_OK) {
        Log::error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db_)));
        return nullptr;
    }

    return stmt;
}

bool Database::beginTransaction() {
    return execute("BEGIN TRANSACTION;");
}

bool Database::commit() {
    return execute("COMMIT;");
}

bool Database::rollback() {
    return execute("ROLLBACK;");
}

sqlite3* Database::handle() const {
    return db_;
}
