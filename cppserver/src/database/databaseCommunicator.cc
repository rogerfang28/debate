#include <iostream>
#include <string>
#include <vector>
#include "sqlite3.h"

sqlite3* db = nullptr;

struct Debate {
    int id;
    std::string topic;
};

// Open DB
void openDB(const std::string& filename) {
    if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Database could not be opened: " << sqlite3_errmsg(db) << "\n";
    }
}

// Create table
void createDebateTable() {
    const char* sql =
        "CREATE TABLE IF NOT EXISTS DEBATE("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "USER TEXT NOT NULL,"
        "TOPIC TEXT NOT NULL);";
    sqlite3_exec(db, sql, nullptr, nullptr, nullptr);
}

// Add a debate for a user
int addDebate(const std::string& user, const std::string& topic) {
    const char* sql = "INSERT INTO DEBATE (USER, TOPIC) VALUES (?, ?);";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return -1;

    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, topic.c_str(), -1, SQLITE_TRANSIENT);

    int result = (sqlite3_step(stmt) == SQLITE_DONE)
                   ? (int)sqlite3_last_insert_rowid(db)
                   : -1;
    sqlite3_finalize(stmt);
    return result;
}

// Read all debates for a user
std::vector<Debate> readDebates(const std::string& user) {
    std::vector<Debate> results;
    const char* sql = "SELECT ID, TOPIC FROM DEBATE WHERE USER = ?;";
    sqlite3_stmt* stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return results;

    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Debate d;
        d.id = sqlite3_column_int(stmt, 0);
        d.topic = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        results.push_back(d);
    }
    sqlite3_finalize(stmt);
    return results;
}

void closeDB() {
    if (db) sqlite3_close(db);
    db = nullptr;
}
