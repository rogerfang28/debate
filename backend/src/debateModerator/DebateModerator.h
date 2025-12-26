#pragma once

#include <string>
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"

// database includes
#include "../database/sqlite/Database.h"

#include "../utils/DebateWrapper.h"
class DebateModerator {
public:
    DebateModerator();
    ~DebateModerator();

    moderator_to_vr::ModeratorToVRMessage handleRequest(const std::string& user, debate_event::DebateEvent& event);
    
private:
    void handleDebateEvent(const std::string& user, debate_event::DebateEvent& event);
    moderator_to_vr::ModeratorToVRMessage buildResponseMessage(const std::string& user);
    // database classes
    Database globalDb;
    UserDatabase userDb;
    DebateDatabase debateDb;
    StatementDatabase statementDb;
    DebateMembersDatabase debateMembersDb;
    DebateWrapper debateWrapper;
};
