#include "DeleteClaimHandler.h"
#include "debate.pb.h"
#include "user.pb.h"
#include "../../../utils/Log.h"

void DeleteClaimHandler::DeleteCurrentStatement(const int& user_id, DebateWrapper& debateWrapper) {
    // Get the current user to find their current claim
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    int currentClaimId = userProto.engagement().debating_info().current_claim().id();
    
    Log::debug("[DeleteCurrentStatement] Deleting claim " + std::to_string(currentClaimId) + " for user: " + std::to_string(user_id));
    
    // Authorization: user can only delete their own claims
    debate::Claim claim = debateWrapper.getClaimById(currentClaimId);
    if (claim.creator_id() != user_id) {
        Log::warn("[DeleteCurrentStatement] Unauthorized delete — user " + std::to_string(user_id) + " tried to delete claim " + std::to_string(currentClaimId));
        return;
    }
    
    // Delete the current claim
    debateWrapper.deleteClaim(currentClaimId, user_id);
}

void DeleteClaimHandler::DeleteChildClaim(const int& claimId, const int& user_id, DebateWrapper& debateWrapper) {
    // Authorization is performed inside deleteClaim (ownership check)
    debateWrapper.deleteClaim(claimId, user_id);
}
