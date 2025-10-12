#include "enterDebate.h"
#include "../../UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include <iostream>

bool enterDebate(const std::string& user, const std::string& topicID) {
    std::string newtopicID = "16"; // Temporary override for testing
    std::cout << "[EnterDebate] User " << user << " entering debate topic with id: " << newtopicID << std::endl;

    try {
        // Create database handler
        UserDatabaseHandler userDbHandler(utils::getDatabasePath());
        
        // Update user's location to the topic
        bool success = userDbHandler.updateUserLocation(user, newtopicID);
        
        if (success) {
            std::cout << "[EnterDebate] Successfully moved user " << user << " to topic " << newtopicID << std::endl;
        } else {
            std::cerr << "[EnterDebate][ERR] Failed to update user location for " << user << std::endl;
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "[EnterDebate][ERR] Exception: " << e.what() << std::endl;
        return false;
    }
}
