#pragma once

#include <string>
#include "moderator_to_vr.pb.h"
#include "debate_event.pb.h"

// database includes
#include "../database/sqlite/Database.h"
#include "../database/debate/DatabaseWrapper.h"

#include "../utils/DebateWrapper.h"
class DebateModerator {
public:
    // usersDb is the shared users.sqlite3 connection (owned by VirtualRenderer)
    // so the moderator and the renderer read/write the same user records.
    explicit DebateModerator(Database& usersDb);
    ~DebateModerator();

    moderator_to_vr::ModeratorToVRMessage handleRequest(debate_event::DebateEvent& event);
    int getUserId(const std::string& username) {
         return dbWrapper.users.getUserId(username);
     }
     DebateWrapper& getDebateWrapper() { return debateWrapper; }
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
