#include "DeleteClaimHandler.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/Log.h"

void DeleteClaimHandler::DeleteCurrentStatement(const int& user_id, DebateWrapper& debateWrapper) {
    // Get the current user to find their current claim
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();
    
    Log::debug("[DeleteCurrentStatement] Deleting claim " + currentClaimId + " for user: " + std::to_string(user_id));
    
    // Delete the current claim
    debateWrapper.deleteClaim(currentClaimId);
}

void DeleteClaimHandler::DeleteChildClaim(const std::string& claimId, const int& user_id, DebateWrapper& debateWrapper) {
    debateWrapper.deleteClaim(claimId);
}
