#pragma once

#include "debate_event.pb.h"
#include "../database/debate/DatabaseWrapper.h"
#include "../utils/DebateWrapper.h"

#include <vector>

// ---------------------------------------
// MoveLogger
// ---------------------------------------
// Translates debate events into move-log entries. This is the ONLY place that
// decides what counts as a move and what it targets -- no handler and no part
// of DebateWrapper knows the log exists.
//
// That isolation is the point. The log currently runs in parallel: nothing
// reads it, and deleting these two files plus the two calls in DebateModerator
// removes it completely without touching a single line of debate logic.
//
// WHY THERE ARE TWO CALLS, NOT ONE
//
// An event describes INTENT; the log records OUTCOME. Neither can be read at a
// single moment:
//
//   * Deletions must be read BEFORE dispatch. Afterwards the row is gone and
//     its debate is unknowable.
//   * Creations must be read AFTER dispatch. Beforehand the id does not exist.
//
// So capture() runs first and snapshots what is about to disappear (and what a
// diff will need), then logForEvent() runs after and works out what appeared.
//
// KNOWN FRAGILITY
//
// Because handlers do not report what they created, this file re-derives it --
// a new child claim is found by diffing the parent's children, a new challenge
// claim by reading where the handler left the user's cursor. That is indirect:
// if a handler changes what it sets, this file can log a wrong id silently
// rather than failing to compile. The trade is deliberate (zero coupling into
// debate logic) and is what the move-log unit tests are there to pin down.
// ---------------------------------------
namespace MoveLogger {

// Facts gathered before an event is dispatched. Only populated for events that
// actually produce a move; everything else leaves this inert.
struct PreState {
    bool relevant = false;

    int debateId = 0;

    // Deletion targets, resolved while the rows still exist.
    int targetClaimId = 0;
    int targetRelationId = 0;

    // Creation context: the parent whose children are about to change, and the
    // children it had beforehand, so the new one can be identified by
    // difference rather than by guessing at ids.
    int parentClaimId = 0;
    std::vector<int> childrenBefore;

    // Concede context, resolved before any cascade runs.
    int challengeRelationId = 0;
    int challengedClaimId = 0;
    int challengingClaimId = 0;

    // Debates the user belonged to beforehand. CREATE_DEBATE does NOT move the
    // user into the debate it creates, so the new debate can only be found by
    // difference -- not by reading where the user ended up.
    std::vector<int> debateIdsBefore;
};

// Runs BEFORE the event is dispatched. Never mutates anything.
PreState capture(const debate_event::DebateEvent& event,
                 const int& user_id,
                 DebateWrapper& debateWrapper);

// Runs AFTER the event is dispatched. Appends zero, one or two moves.
//
// Never throws and never reports failure: a log write must not be able to break
// an operation that already succeeded. Failures are logged and dropped.
void logForEvent(const debate_event::DebateEvent& event,
                 const int& user_id,
                 const PreState& pre,
                 DebateWrapper& debateWrapper,
                 DatabaseWrapper& databaseWrapper);

}  // namespace MoveLogger
