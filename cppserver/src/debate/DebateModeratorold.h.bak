#ifndef DEBATEMODERATOR_H
#define DEBATEMODERATOR_H

#include <string>
#include <iostream>
#include "../database/databaseCommunicator.h"  // for addDebate, readDebates, deleteDebateTopic

class DebateModerator {
public:
    DebateModerator();
    ~DebateModerator();

    // Add and clear debate topics for a user
    void handleAddDebate(const std::string& user, const std::string& debateTopic);
    void handleClearDebates(const std::string& user);

private:
    // Helper to print current debates for a user
    void printCurrentDebates(const std::string& user);
};

#endif // DEBATEMODERATOR_H
