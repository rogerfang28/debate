#pragma once

#include "../debateModerator/DebateModerator.h"
#include <string>

// Turns generated JSON into a real debate: real claims, real relations, real
// moves, created through the same handlers the UI uses.
//
// Everything goes through DebateModerator::handleRequest, the same entry point
// the UI posts to -- not the handlers underneath it. That distinction is not
// cosmetic: MoveLogger hangs off event dispatch, so calling handlers directly
// creates claims that never reach the move log, and statuses are computed FROM
// the move log. A debate built that way looks right and is inert.
class GeneratedDebateBuilder {
public:
    struct Result {
        bool ok = false;
        int debateId = 0;
        int claimsCreated = 0;
        int argumentsCreated = 0;
        // How many multi-claim groups were split because they carried no
        // justification. Surfaced because it says something about the
        // generation, not just the build.
        int groupsSplit = 0;
        std::string error;
    };

    // `json` is DebateGenerator output. Validates before touching anything:
    // a debate half-built from bad JSON is worse than none, since the user
    // then has to find and delete it.
    static Result Build(const std::string& json, int user_id, DebateModerator& moderator);
};
