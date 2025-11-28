#include "../../database/handlers/UserDatabaseHandler.h"
#include "../../database/handlers/DebateDatabaseHandler.h"
#include "BuildPayload.h"

moderator_to_vr::ModeratorToVRMessage build(const std::string& username) {
    moderator_to_vr::ModeratorToVRMessage message;

    // hard code for now
    userengagement::UserEngagement engagement;
    // engagement.set_user_id(username);
    engagement.set_current_action(userengagement::ACTION_NONE);

    return message;
}