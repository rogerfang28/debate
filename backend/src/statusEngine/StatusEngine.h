#pragma once

#include "debate.pb.h"

#include <map>
#include <string>
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

// ---------------------------------------
// PARITY MODE
// ---------------------------------------
// Reproduces what the EXISTING engine would store, from the move log alone.
//
// This is not the target design -- computeStatuses above is. It exists to
// answer one question first: does the log carry enough information to
// reconstruct today's behaviour? Proving that before changing any behaviour
// means a later difference is a decision, not an accident.
//
// So it deliberately copies the old model warts and all: three values, keyed
// per user, and the same Rule A/B/C/D fixed point that DebateWrapper runs.
//
// EXACT PARITY IS NOT ALWAYS POSSIBLE, and the reason is worth knowing.
// ComputeStatusForUser begins with `ns = currentStatus` -- the value already
// stored -- and only overrides it when a rule fires, and PropagateClaimStatuses
// never resets before iterating. So the old engine's output depends on the
// accumulated history of every propagation run against that debate, not on the
// graph. A pure function cannot reproduce state the log never recorded. This
// seeds from the log and runs the same rules, which should agree for any
// debate whose history is a clean forward sequence; where it disagrees, the
// case is either a rule reproduced wrongly here or a claim whose stored status
// is stale, and both are worth finding.
struct LegacyResult {
    // username -> claim id -> status, mirroring Claim.user_statuses.
    std::map<std::string, std::map<int, debate::ClaimStatus>> by_user;
};

LegacyResult computeLegacyStatuses(const std::vector<debate::Claim>& claims,
                                   const std::vector<debate::Relationship>& relations,
                                   const std::vector<debate::Move>& moves,
                                   const std::map<int, std::string>& usernames);

}  // namespace StatusEngine
