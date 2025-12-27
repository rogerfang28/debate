#include "SubmitEditClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../CancelEditClaim/CancelEditClaim.h"
#include "../../../utils/Log.h"

void SubmitEditClaim::Submit(const std::string& user, const std::string& newClaim, DebateWrapper& debateWrapper) {
    // update the claim text in the debate wrapper
    // find current claim
    std::vector<uint8_t> userData = debateWrapper.getUserProtobufByUsername(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();

    // change the claim text
    debateWrapper.editClaimText(currentClaimId, newClaim);

    // close
    Log::debug("[SubmitEditClaim] Closing edit claim for user: " + user);
    CancelEditClaim::Cancel(user, debateWrapper);
}
