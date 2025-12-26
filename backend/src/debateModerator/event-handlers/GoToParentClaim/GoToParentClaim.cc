#include "GoToParentClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"
#include "../GoToClaim/GoToClaim.h"

void GoToParentClaimHandler::GoToParentClaim(const std::string& user, DebateWrapper& debateWrapper) {
    std::vector<uint8_t> userData = debateWrapper.getUserProtobufByUsername(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();
    // find parent claim
    debate::Claim parentClaim = debateWrapper.findClaimParent(currentClaimId);
    // use go to claim handler to go to parent claim
    GoToClaimHandler::GoToClaim(parentClaim.id(), user, debateWrapper);
}