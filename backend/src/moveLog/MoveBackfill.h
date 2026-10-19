#pragma once

#include "../database/debate/DatabaseWrapper.h"
#include "../utils/DebateWrapper.h"

#include <string>
#include <vector>

// ---------------------------------------
// MoveBackfill
// ---------------------------------------
// Writes a move log for a debate that has none.
//
// Debates made before the log existed have only their stored per-user
// statuses. Once the page renders from the log alone, those debates would
// show as though nobody had ever taken a position. This writes the receipts
// they never got: an ASSERT per claim, an ASSERT per support relation, an
// OPPOSE per challenge -- all read off the graph -- and then whatever the
// stored statuses still say that the replay of those does not: an ACCEPT
// where someone holds a claim TRUE without having made it, a CONCEDE where
// an author holds their own challenged claim FALSE. What no move can say is
// reported, not invented.
//
// Every backfilled move carries "backfilled":1 in its payload, so it can be
// told from a move a person actually made. Runs once per debate: a debate
// with any move at all is left alone.
// ---------------------------------------
namespace MoveBackfill {

struct Report {
    int debatesTouched = 0;
    int movesWritten = 0;
    // "debate 3 claim 7 user bob: stored FALSE, no move can say it"
    std::vector<std::string> unrepresentable;
};

// Backfills one debate if it has no moves. Returns what was written.
Report BackfillDebate(int debateId, DebateWrapper& debateWrapper, DatabaseWrapper& db);

// Backfills every debate in the store that has no moves.
Report BackfillAll(DebateWrapper& debateWrapper, DatabaseWrapper& db);

}  // namespace MoveBackfill
