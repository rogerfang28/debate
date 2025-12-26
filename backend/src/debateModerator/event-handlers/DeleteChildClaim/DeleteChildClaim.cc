#include "DeleteChildClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void DeleteChildClaimHandler::DeleteChildClaim(const std::string& claimId, const std::string& user, DebateWrapper& debateWrapper) {
    debateWrapper.deleteClaim(claimId);
}