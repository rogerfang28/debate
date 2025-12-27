#include "GoToParentClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"
#include "../GoToClaim/GoToClaim.h"

void GoToParentClaimHandler::GoToParentClaim(const std::string& user, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();
    // find parent claim
    debate::Claim parentClaim = debateWrapper.findClaimParent(currentClaimId);
    // use go to claim handler to go to parent claim
    GoToClaimHandler::GoToClaim(parentClaim.id(), user, debateWrapper);
}