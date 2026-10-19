#pragma once

#include "../debateModerator/DebateModerator.h"
#include "extraction.pb.h"

#include <string>

// Turns an extracted TREE into a real debate.
//
// Replaces GeneratedDebateBuilder (flat JSON with named targets) and the
// scenario path (a step list the model had to number itself). Both made the
// model track claim identity across its own output; a tree does not, because
// the parent of a claim is the node containing it.
//
// Everything is created through DebateModerator::handleRequest, the same entry
// point the UI posts to -- not the handlers underneath it. MoveLogger hangs off
// event dispatch, so calling handlers directly creates claims that never reach
// the move log, and statuses are computed FROM the move log. A debate built
// that way looks right and is inert.
//
// The whole tree is validated before anything is written. A debate half-built
// from a bad tree is worse than none: the user then has to find and delete it.
class ExtractedDebateBuilder {
public:
    struct Result {
        bool ok = false;
        int debateId = 0;
        int claimsCreated = 0;
        int argumentsCreated = 0;
        int maxDepthReached = 0;
        // Groups of several claims that were split into independent arguments
        // for carrying no joint_reason. Reported because it says something
        // about the generation, not just about the build.
        int groupsSplit = 0;
        // Arguments dropped for naming a basis the registry does not know.
        // The claims are still created; only the unknown name is discarded.
        int unknownBases = 0;
        std::string error;
    };

    // `text` is protobuf text format for extraction.ExtractedDebate, as the
    // model wrote it. A leading ``` fence is tolerated -- models add one however
    // firmly they are told not to.
    static Result Build(const std::string& text, int user_id, DebateModerator& moderator);

    // Parses and validates without writing anything. Exposed so a caller -- and
    // a test -- can tell "the model wrote nonsense" apart from "the write
    // failed", which are different problems with different fixes.
    static bool Parse(const std::string& text, extraction::ExtractedDebate& out,
                      std::string& error);
};
