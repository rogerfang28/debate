#pragma once

#include <string>
#include "../database/handlers/DebateDatabaseHandler.h"

class DebateModerator {
public:
    DebateModerator();
    ~DebateModerator();

    // Add and clear debate topics for a user
    void handleAddDebate(const std::string& user, const std::string& debateTopic);
    void handleClearDebates(const std::string& user);

private:
    DebateDatabaseHandler dbHandler;  // internal handler instance

    // Helper to print current debates for a user
    void printCurrentDebates(const std::string& user);
};
