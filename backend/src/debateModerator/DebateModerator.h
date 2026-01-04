#pragma once

#include <string>
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"

// database includes
#include "../database/sqlite/Database.h"
#include "../database/handlers/DatabaseWrapper.h"

#include "../utils/DebateWrapper.h"
class DebateModerator {
public:
    DebateModerator();
    ~DebateModerator();

    moderator_to_vr::ModeratorToVRMessage handleRequest(debate_event::DebateEvent& event);
    int getUserId(const std::string& username) {
        return dbWrapper.users.getUserId(username);
    }
    int createUserIfNotExist(const std::string& username);
    
private:
    void handleDebateEvent(const int& user_id, debate_event::DebateEvent& event);
    moderator_to_vr::ModeratorToVRMessage buildResponseMessage(const int& user_id);
    int validateAuth(debate_event::DebateEvent& event);
    // database classes
    Database globalDb;
    DatabaseWrapper dbWrapper;
    DebateWrapper debateWrapper;
};
