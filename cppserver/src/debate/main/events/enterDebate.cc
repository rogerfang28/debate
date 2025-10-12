#include "enterDebate.h"
#include "../../UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include <iostream>

bool enterDebate(const std::string& user, const std::string& topicID) {
    std::cout << "[EnterDebate] User " << user << " entering debate topic with id: " << topicID << std::endl;

    try {
        // Create database handler
        UserDatabaseHandler userDbHandler(utils::getDatabasePath());
        
        // Update user's location to the topic
        bool success = userDbHandler.updateUserLocation(user, topicID);
        
        if (success) {
            std::cout << "[EnterDebate] Successfully moved user " << user << " to topic " << topicID << std::endl;
        } else {
            std::cerr << "[EnterDebate][ERR] Failed to update user location for " << user << std::endl;
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "[EnterDebate][ERR] Exception: " << e.what() << std::endl;
        return false;
    }
}
