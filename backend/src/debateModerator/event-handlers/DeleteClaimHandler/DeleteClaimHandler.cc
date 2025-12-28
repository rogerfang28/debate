#include "DeleteClaimHandler.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/Log.h"

void DeleteClaimHandler::DeleteCurrentStatement(const std::string& user, DebateWrapper& debateWrapper) {
    // Get the current user to find their current claim
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();
    
    Log::debug("[DeleteCurrentStatement] Deleting claim " + currentClaimId + " for user: " + user);
    
    // Delete the current claim
    debateWrapper.deleteClaim(currentClaimId);
}

void DeleteClaimHandler::DeleteChildClaim(const std::string& claimId, const std::string& user, DebateWrapper& debateWrapper) {
    debateWrapper.deleteClaim(claimId);
}
