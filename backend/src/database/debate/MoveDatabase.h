#pragma once

#include "../sqlite/Database.h"
#include <string>

// ---------------------------------------
// Class: MoveDatabase
// ---------------------------------------
// Append-only log of debate actions.
//
// This table is currently a PARALLEL RECORD ONLY -- nothing reads from it. It
// exists so that the move log is complete from the moment the status engine is
// ready to consume it, rather than needing a backfill later.
//
// Two rules this class exists to enforce:
//
//   1. Append-only. There is no update and no delete, deliberately. A
//      correction is a new move, never a mutation of an old one.
//
//   2. Failure here must never break a working feature. appendMove returns
//      false and logs rather than throwing, and every caller ignores the
//      result. A debate action must succeed even if its log write does not.
//
// Moves are DEBATE-scoped, not claim-scoped: seq is monotonic within a debate,
// and is assigned server-side.
// ---------------------------------------
class MoveDatabase {
public:
    explicit MoveDatabase(Database& db) : db_(db) {}

    bool ensureTable();

    // Append one move. seq is assigned server-side as MAX(seq)+1 for the given
    // debate, computed inside the INSERT so the read and the write cannot be
    // interleaved by a concurrent writer.
    //
    // targetType is "claim" or "relation". payload is free-form JSON, or empty.
    // Returns false on failure -- callers are expected to ignore it and carry
    // on, since this log must not be able to break an existing operation.
    bool appendMove(int debateId,
                    int actorId,
                    const std::string& type,
                    const std::string& targetType,
                    int targetId,
                    const std::string& payload);

    // Read helpers. Not used by any feature -- these exist for verification and
    // debugging while the log runs in parallel.
    int countMovesForDebate(int debateId);
    int lastSeqForDebate(int debateId);

private:
    Database& db_;
};
