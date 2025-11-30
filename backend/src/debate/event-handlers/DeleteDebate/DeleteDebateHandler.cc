#include "DeleteDebateHandler.h"

#include "../../../database/handlers/DebateDatabaseHandler.h"
#include "../../../utils/pathUtils.h"

void DeleteDebateHandler::DeleteDebate(const std::string& debate_id, const std::string& user) {
    DebateDatabaseHandler dbHandler(utils::getDatabasePath());

    std::cout << "[DeleteDebateHandler] DeleteDebate called for user: "
              << user << ", debate ID: " << debate_id << std::endl;

    bool ok = dbHandler.removeDebate(debate_id, user);

    if (ok) {
        std::cout << "[DeleteDebateHandler] Successfully deleted debate ID: " 
                  << debate_id << " for user: " << user << std::endl;
    } else {
        std::cerr << "[DeleteDebateHandler] Failed to delete debate ID: " 
                  << debate_id << " for user: " << user << std::endl;
    }
}