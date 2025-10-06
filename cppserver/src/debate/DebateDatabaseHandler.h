#ifndef DEBATEDATABASEHANDLER_H
#define DEBATEDATABASEHANDLER_H

#include <string>
#include <vector>
#include <map>
#include <iostream>
#include "../database/databaseCommunicator.h"

// ---------------------------------------
// Class: DebateDatabaseHandler
// ---------------------------------------
// Provides a clean object-oriented interface
// for managing debate topics in the database.
// ---------------------------------------
class DebateDatabaseHandler {
public:
    // Constructor automatically ensures the table exists
    DebateDatabaseHandler(const std::string& dbFile = "debates.sqlite3");

    // Add a debate for a given user
    int addDebate(const std::string& user, const std::string& topic);

    // Retrieve all debates for a specific user
    std::vector<std::map<std::string, std::string>> getDebates(const std::string& user);

    // Remove a specific debate for a user
    bool removeDebate(const std::string& topic, const std::string& user);

    // Clear all debates for a given user
    bool clearUserDebates(const std::string& user);

private:
    std::string dbFilename;
    void ensureTable();  // create table if not exists
};

#endif // DEBATEDATABASEHANDLER_H
