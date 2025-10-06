#include "DebateModerator.h"
#include "../database/databaseCommunicator.h"  // for addDebate, readDebates, deleteDebateTopic
#include <iostream>
#include <vector>

DebateModerator::DebateModerator() {
    std::cout << "[DebateModerator] Initialized.\n";
}

DebateModerator::~DebateModerator() {
    std::cout << "[DebateModerator] Destroyed.\n";
}

// Add a debate topic for a specific user
void DebateModerator::handleAddDebate(const std::string& user, const std::string& debateTopic) {
    std::cout << "[DebateModerator] handleAddDebate called for user: " 
              << user << ", topic: " << debateTopic << std::endl;

    int newId = addDebate(user, debateTopic);
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

    auto debates = readDebates(user);
    for (const auto& d : debates) {
        std::cout << "[DebateModerator] Deleting debate topic: " << d.topic << std::endl;
        deleteDebateTopic(d.topic, user);
    }

    std::cout << "[DebateModerator] All debates cleared for user: " << user << std::endl;
}

// Print all current debates for a user
void DebateModerator::printCurrentDebates(const std::string& user) {
    auto debates = readDebates(user);
    std::cout << "[DebateModerator] Current debates for user '" << user << "':\n";
    for (const auto& d : debates) {
        std::cout << " - " << d.id << " : " << d.topic << std::endl;
    }
}
