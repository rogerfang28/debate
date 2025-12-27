#include "GoToClaim.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../utils/pathUtils.h"

void GoToClaimHandler::GoToClaim(const std::string& claim_id, const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    // update the current claim id to claim_id
    // save back to database;
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(claim_id);
    debateWrapper.updateUserProtobuf(user, userProto);
}