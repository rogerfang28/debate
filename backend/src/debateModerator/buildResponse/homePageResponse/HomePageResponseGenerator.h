#ifndef HOMEPAGERESPONSEGENERATOR_H
#define HOMEPAGERESPONSEGENERATOR_H

#include "moderator_to_vr.pb.h"
#include "../../../utils/DebateWrapper.h"
#include <map>
#include <string>

class HomePageResponseGenerator {
public:
    static void BuildHomePageResponse(
        moderator_to_vr::ModeratorToVRMessage& responseMessage,
        const int& user_id,
        DebateWrapper& debateWrapper,
        // Debate id -> its computed card. Optional: building every card
        // means building every debate's collection (~0.8 s for ~56 debates),
        // so the moderator keeps the cards between requests and clears them
        // whenever an event may have changed a debate.
        std::map<int, moderator_to_vr::DebateTopic>* cardCache = nullptr
    );
};

#endif // HOMEPAGERESPONSEGENERATOR_H
