#include "ClearDebatesHandler.h"

#include "../../../database/handlers/DebateDatabaseHandler.h"
#include "../../../utils/pathUtils.h"

void ClearDebatesHandler::ClearDebates(const std::string& user) {
    DebateDatabaseHandler dbHandler(utils::getDatabasePath());

    std::cout << "[ClearDebatesHandler] ClearDebates called for user: "
              << user << std::endl;

    bool ok = dbHandler.clearUserDebates(user);

    if (ok) {
        std::cout << "[ClearDebatesHandler] All debates cleared for user: " 
                  << user << std::endl;
    } else {
        std::cerr << "[ClearDebatesHandler] Failed to clear debates for user: " 
                  << user << std::endl;
    }
}