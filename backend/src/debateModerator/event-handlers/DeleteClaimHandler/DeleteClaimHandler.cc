#include "DeleteClaimHandler.h"
#include "user.pb.h"
#include "../../../utils/Log.h"

// The move log is written HERE rather than inside DebateWrapper::deleteClaim
// because that function is also used to tear a whole debate down. Deleting a
// debate destroys its move log too, so there is nothing to reconstruct into and
// nothing worth logging -- hooking the lower-level function would emit a RETRACT
// per claim during teardown.
//
// Note the log says RETRACT while the row is still being destroyed, exactly as
// it is today. That inconsistency is deliberate for the parallel phase: it buys
// a complete log now, and the row deletion goes away once the engine takes over.
// Deletion behaviour itself is unchanged.

void DeleteClaimHandler::DeleteCurrentStatement(const int& user_id, DebateWrapper& debateWrapper) {
    // Get the current user to find their current claim
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();

    Log::debug("[DeleteCurrentStatement] Deleting claim " + std::to_string(currentClaimId) + " for user: " + std::to_string(user_id));

    // Resolve the debate before the claim is gone -- afterwards it is unfindable.
    const int debateId = debateWrapper.findDebateId(currentClaimId);
    debateWrapper.logMove(debateId, user_id, "RETRACT", /*targetIsClaim=*/true, currentClaimId);

    // Delete the current claim
    debateWrapper.deleteClaim(currentClaimId);
}

void DeleteClaimHandler::DeleteChildClaim(const int& claimId, const int& user_id, DebateWrapper& debateWrapper) {
    // Resolve the debate before the claim is gone -- afterwards it is unfindable.
    const int debateId = debateWrapper.findDebateId(claimId);
    debateWrapper.logMove(debateId, user_id, "RETRACT", /*targetIsClaim=*/true, claimId);

    debateWrapper.deleteClaim(claimId);
}
