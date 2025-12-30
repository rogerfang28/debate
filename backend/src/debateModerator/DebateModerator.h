#pragma once

#include <string>
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"

// database includes
#include "../database/sqlite/Database.h"
#include "../database/handlers/UserDatabase.h"
#include "../database/handlers/DebateDatabase.h"
#include "../database/handlers/StatementDatabase.h"
#include "../database/handlers/DebateMembersDatabase.h"
#include "../database/handlers/LinkDatabase.h"

#include "../utils/DebateWrapper.h"
class DebateModerator {
public:
    DebateModerator();
    ~DebateModerator();

    moderator_to_vr::ModeratorToVRMessage handleRequest(const int& user_id, debate_event::DebateEvent& event);
    
private:
    void handleDebateEvent(const int& user_id, debate_event::DebateEvent& event);
    moderator_to_vr::ModeratorToVRMessage buildResponseMessage(const int& user_id);
    // database classes
    Database globalDb;
    UserDatabase userDb;
    DebateDatabase debateDb;
    StatementDatabase statementDb;
    DebateMembersDatabase debateMembersDb;
    LinkDatabase linkDb;
    DebateWrapper debateWrapper;
};
