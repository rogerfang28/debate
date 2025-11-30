#include "ClearDebatesHandler.h"

#include "../../../database/handlers/DebateDatabaseHandler.h"
#include "../../../utils/pathUtils.h"

void ClearDebatesHandler::ClearDebates(const std::string& user_id) {
    DebateDatabaseHandler dbHandler(utils::getDatabasePath());

    std::cout << "[ClearDebatesHandler] ClearDebates called for user: "
              << user_id << std::endl;

    bool ok = dbHandler.clearUserDebates(user_id);

    if (ok) {
        std::cout << "[ClearDebatesHandler] All debates cleared for user: " 
                  << user_id << std::endl;
    } else {
        std::cerr << "[ClearDebatesHandler] Failed to clear debates for user: " 
                  << user_id << std::endl;
    }
}