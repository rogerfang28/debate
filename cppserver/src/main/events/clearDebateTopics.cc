#include "clearDebateTopics.h"
#include <iostream>
#include "../../database/databaseCommunicator.h"

void clearDebateTopics(const std::string& user) {
    // Implementation to clear all debate topics for the specified user
    // This function can be implemented similarly to deleteDebateTopic
    // but will remove all topics associated with the user.
    std::cout << "[ClearDebateTopics] clearDebateTopics function called for user: " << user << std::endl;
    // Actual database logic to delete all topics for the user would go here.
    auto debates = readDebates(user);
    for (const auto& d : debates) {
        std::cout << "[ClearDebateTopics] Deleting debate topic: " << d.topic << std::endl;
        deleteDebateTopic(d.topic, user);
    }
    return;
}