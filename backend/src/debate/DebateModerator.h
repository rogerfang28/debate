#pragma once

#include <string>
#include "../database/handlers/DebateDatabaseHandler.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"

class DebateModerator {
public:
    DebateModerator();
    ~DebateModerator();

    // Add and clear debate topics for a user
    moderator_to_vr::ModeratorToVRMessage handleRequest(const std::string& user, debate_event::DebateEvent& event);
    
private:
    // DebateDatabaseHandler dbHandler;  // internal handler instance
    void handleDebateEvent(const std::string& user, debate_event::DebateEvent& event);
    moderator_to_vr::ModeratorToVRMessage buildResponseMessage();
};
