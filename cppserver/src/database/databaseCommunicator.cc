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
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        std::cerr << "Error creating table: " << errMsg << "\n";
        sqlite3_free(errMsg);
    }
}


// Add a debate for a user
int addDebate(const std::string& user, const std::string& topic) {
    openDB("debates.sqlite3");      // open or create DB file
    createDebateTable();            // make sure table exists
    const char* sql = "INSERT INTO DEBATE (USER, TOPIC) VALUES (?, ?);";
    sqlite3_stmt* stmt;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "Prepare failed: " << sqlite3_errmsg(db) << "\n";
        return -1;
    }

    sqlite3_bind_text(stmt, 1, user.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, topic.c_str(), -1, SQLITE_TRANSIENT);

    int rc = sqlite3_step(stmt);
    if (rc != SQLITE_DONE) {
        std::cerr << "Insert failed: " << sqlite3_errmsg(db) << "\n";
        sqlite3_finalize(stmt);
        return -1;
    }

    int rowid = (int)sqlite3_last_insert_rowid(db);
    sqlite3_finalize(stmt);
    
    closeDB();  // clean up
    return rowid;
}


// Read all debates for a user
std::vector<Debate> readDebates(const std::string& user) {
    openDB("debates.sqlite3");      // open or create DB file
    createDebateTable();            // make sure table exists
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
    closeDB();  // clean up
    return results;
}

void closeDB() {
    if (db) sqlite3_close(db);
    db = nullptr;
}

// int main() {
//     openDB("debates.sqlite3");
//     createDebateTable();

//     int id = addDebate("defaultUser", "THIS IS FDEBATE");
//     std::cout << "Inserted ID: " << id << std::endl;

//     auto debates = readDebates("defaultUser");
//     std::cout << "Found " << debates.size() << " debates\n";
//     for (const auto& d : debates) {
//         std::cout << "[" << d.id << "] " << d.topic << std::endl;
//     }
//     std::vector<std::string> list_of_debates;
//     for(int i = 0; i < debates.size(); i++){
//         list_of_debates.push_back(debates[i].topic);
//         std::cout << list_of_debates[i] << std::endl;
//     }
//     closeDB();
// }
