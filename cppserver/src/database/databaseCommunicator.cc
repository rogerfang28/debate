#include "databaseCommunicator.h"
#include <iostream>
#include <string>
#include <vector>
#include "sqlite3.h"

sqlite3* db = nullptr;

// RAII wrapper for statement cleanup
struct Statement {
    sqlite3_stmt* stmt = nullptr;
    ~Statement() {
        if (stmt) sqlite3_finalize(stmt);
    }
};

// Open database
void openDB(const std::string& filename) {
    if (sqlite3_open(filename.c_str(), &db) != SQLITE_OK) {
        std::cerr << "Database could not be opened: " << sqlite3_errmsg(db) << "\n";
        exit(1);
    }
}

// Create DEBATE table
void createDebateTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS DEBATE(
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            TOPIC TEXT NOT NULL
        );
    )";

    char* errMsg = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::cerr << "Error creating DEBATE table: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}

// Insert a debate topic, return inserted ID
int insertDebate(const std::string& topic) {
    const char* sql = "INSERT INTO DEBATE (TOPIC) VALUES (?);";
    Statement s;

    if (sqlite3_prepare_v2(db, sql, -1, &s.stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << "\n";
        return -1;
    }

    sqlite3_bind_text(s.stmt, 1, topic.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(s.stmt) != SQLITE_DONE) {
        std::cerr << "Insert failed: " << sqlite3_errmsg(db) << "\n";
        return -1;
    }

    return (int)sqlite3_last_insert_rowid(db);
}

// Read all debates
std::vector<Debate> readDebates() {
    std::vector<Debate> results;
    const char* sql = "SELECT ID, TOPIC FROM DEBATE;";
    Statement s;

    if (sqlite3_prepare_v2(db, sql, -1, &s.stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << "\n";
        return results;
    }

    while (sqlite3_step(s.stmt) == SQLITE_ROW) {
        Debate d;
        d.id = sqlite3_column_int(s.stmt, 0);
        d.topic = reinterpret_cast<const char*>(sqlite3_column_text(s.stmt, 1));
        results.push_back(d);
    }

    return results;
}

// Search debates by keyword
std::vector<Debate> searchDebates(const std::string& keyword) {
    std::vector<Debate> results;
    const char* sql = "SELECT ID, TOPIC FROM DEBATE WHERE TOPIC LIKE ?;";
    Statement s;

    if (sqlite3_prepare_v2(db, sql, -1, &s.stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << "\n";
        return results;
    }

    std::string pattern = "%" + keyword + "%";
    sqlite3_bind_text(s.stmt, 1, pattern.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(s.stmt) == SQLITE_ROW) {
        Debate d;
        d.id = sqlite3_column_int(s.stmt, 0);
        d.topic = reinterpret_cast<const char*>(sqlite3_column_text(s.stmt, 1));
        results.push_back(d);
    }

    return results;
}

void closeDB() {
    if (db) sqlite3_close(db);
    db = nullptr;
}

// int main() {
//     openDB("debates.sqlite3");
//     createDebateTable();

//     // Insert debates
//     int id1 = insertDebate("Should artificial intelligence be regulated?");
//     int id2 = insertDebate("Is universal basic income a good idea?");
//     int id3 = insertDebate("Should governments ban fossil fuels?");

//     std::cout << "Inserted IDs: " << id1 << ", " << id2 << ", " << id3 << "\n\n";

//     // Read all debates
//     auto debates = readDebates();
//     std::cout << "All debates:\n";
//     for (const auto& d : debates) {
//         std::cout << "[" << d.id << "] " << d.topic << "\n";
//     }

//     // Search debates
//     auto found = searchDebates("income");
//     std::cout << "\nSearch results for 'income':\n";
//     for (const auto& d : found) {
//         std::cout << "[" << d.id << "] " << d.topic << "\n";
//     }

//     sqlite3_close(db);
//     return 0;
// }
