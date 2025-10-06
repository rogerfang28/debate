#include "DebateModerator.h"
#include "./DebateDatabaseHandler.h"   // now use the new handler
#include <iostream>
#include <vector>

DebateModerator::DebateModerator()
    : dbHandler("debates.sqlite3") // initialize handler with database
{
    std::cout << "[DebateModerator] Initialized.\n";
}

DebateModerator::~DebateModerator() {
    std::cout << "[DebateModerator] Destroyed.\n";
}

// Add a debate topic for a specific user
void DebateModerator::handleAddDebate(const std::string& user, const std::string& debateTopic) {
    std::cout << "[DebateModerator] handleAddDebate called for user: "
              << user << ", topic: " << debateTopic << std::endl;

    int newId = dbHandler.addDebate(user, debateTopic);
    if (newId == -1) {
        std::cerr << "[DebateModerator] Failed to insert debate topic.\n";
    } else {
        std::cout << "[DebateModerator] Inserted debate with ID: " << newId << std::endl;
    }

    printCurrentDebates(user);
}

// Clear all debate topics for a user
void DebateModerator::handleClearDebates(const std::string& user) {
    std::cout << "[DebateModerator] handleClearDebates called for user: " << user << std::endl;
    bool ok = dbHandler.clearUserDebates(user);

    if (ok)
        std::cout << "[DebateModerator] All debates cleared for user: " << user << std::endl;
    else
        std::cerr << "[DebateModerator] Failed to clear debates for user: " << user << std::endl;
}

// Print all current debates for a user
void DebateModerator::printCurrentDebates(const std::string& user) {
    auto debates = dbHandler.getDebates(user);
    std::cout << "[DebateModerator] Current debates for user '" << user << "':\n";
    if (debates.empty()) {
        std::cout << " (none)\n";
        return;
    }

    for (const auto& row : debates) {
        std::cout << " - " << row.at("ID") << " : " << row.at("TOPIC") << std::endl;
    }
}
