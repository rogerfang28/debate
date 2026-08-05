#pragma once

#include "debate.pb.h"

#include <map>
#include <vector>

// ---------------------------------------
// StatusEngine
// ---------------------------------------
// computeStatuses is a PURE function: graph in, statuses out.
//
//   - no database, no file, no clock, no globals
//   - never reads Claim.status or Claim.user_statuses, and never writes them.
//     Those fields are the OLD model; this function exists to replace them, so
//     consulting one would defeat the point and quietly reintroduce the
//     contradiction the move log is meant to remove
//   - same inputs always give the same outputs
//
// Nothing calls this yet. It runs beside the existing engine, not in place of
// it, so it can be built and tested before anything depends on it.
//
// SCOPE
//
// Only the part of the model we actually log today: ASSERT, OPPOSE, CONCEDE,
// RETRACT, producing COLLAPSED and STANDING. The other nine statuses need
// moves we never write or distinctions we never record -- see the notes on
// each below, and do not add a status here without first adding the move that
// justifies it. A status the log cannot support is a guess wearing a verdict's
// clothing.
// ---------------------------------------
namespace StatusEngine {

struct Result {
    // Claim id -> status. Every claim passed in appears exactly once.
    std::map<int, debate::Status> claim_status;
};

// moves need not be pre-sorted; they are ordered by seq internally.
Result computeStatuses(const std::vector<debate::Claim>& claims,
                       const std::vector<debate::Relationship>& relations,
                       const std::vector<debate::Move>& moves);

}  // namespace StatusEngine
