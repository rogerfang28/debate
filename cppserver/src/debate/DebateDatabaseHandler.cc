#include "DebateDatabaseHandler.h"

DebateDatabaseHandler::DebateDatabaseHandler(const std::string& dbFile)
    : dbFilename(dbFile) {
    ensureTable();
}

void DebateDatabaseHandler::ensureTable() {
    if (!openDB(dbFilename)) return;
    createTable("DEBATE", {
        {"USER", "TEXT NOT NULL"},
        {"TOPIC", "TEXT NOT NULL"}
    });
    closeDB();
}

int DebateDatabaseHandler::addDebate(const std::string& user, const std::string& topic) {
    if (!openDB(dbFilename)) return -1;
    int id = insertRow("DEBATE", {"USER", "TOPIC"}, {user, topic});
    closeDB();
    if (id != -1)
        std::cout << "[DebateDB] Added debate: " << topic << " for user " << user << "\n";
    return id;
}

std::vector<std::map<std::string, std::string>>
DebateDatabaseHandler::getDebates(const std::string& user) {
    if (!openDB(dbFilename)) return {};
    auto rows = readRows("DEBATE", "USER = '" + user + "'");
    closeDB();
    std::cout << "[DebateDB] Retrieved " << rows.size()
              << " debates for user " << user << "\n";
    return rows;
}

bool DebateDatabaseHandler::removeDebate(const std::string& topic, const std::string& user) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("DEBATE", "TOPIC = '" + topic + "' AND USER = '" + user + "'");
    closeDB();
    if (ok)
        std::cout << "[DebateDB] Removed debate: " << topic << " for user " << user << "\n";
    return ok;
}

bool DebateDatabaseHandler::clearUserDebates(const std::string& user) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("DEBATE", "USER = '" + user + "'");
    closeDB();
    if (ok)
        std::cout << "[DebateDB] Cleared all debates for user: " << user << "\n";
    return ok;
}
