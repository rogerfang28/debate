#include "SubmitEditClaimDescription.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../CancelEditClaimDescription/CancelEditClaimDescription.h"
#include "../../../utils/Log.h"

void SubmitEditClaimDescriptionHandler::SubmitEditClaimDescription(const std::string& user, const std::string& newDescription, DebateWrapper& debateWrapper) {
    // update the claim description in the debate wrapper
    // find current claim
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    std::string currentClaimId = userProto.engagement().debating_info().current_claim().id();

    // change the description
    debateWrapper.changeDescriptionOfClaim(
        currentClaimId,
        newDescription);

    // close
    Log::debug("[SubmitEditClaimDescription] Closing edit description for user: " + user);
    CancelEditClaimDescriptionHandler::CancelEditClaimDescription(user, debateWrapper);
}