#pragma once

#include <string>
#include "../database/handlers/DebateDatabaseHandler.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"

class DebateModerator {
public:
    DebateModerator();
    ~DebateModerator();

    moderator_to_vr::ModeratorToVRMessage handleRequest(const std::string& user, debate_event::DebateEvent& event);
    
private:
    void handleDebateEvent(const std::string& user, debate_event::DebateEvent& event);
    moderator_to_vr::ModeratorToVRMessage buildResponseMessage();
};
