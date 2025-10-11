#include "goHome.h"
#include "../../UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include <iostream>

bool goHome(const std::string& user) {
    std::cout << "[GoHome] User " << user << " going home" << std::endl;
    
    try {
        // Create database handler
        UserDatabaseHandler userDbHandler(utils::getDatabasePath());
        
        // Update user's location to "home"
        bool success = userDbHandler.updateUserLocation(user, "home");
        
        if (success) {
            std::cout << "[GoHome] Successfully moved user " << user << " to home" << std::endl;
        } else {
            std::cerr << "[GoHome][ERR] Failed to update user location for " << user << std::endl;
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "[GoHome][ERR] Exception: " << e.what() << std::endl;
        return false;
    }
}
