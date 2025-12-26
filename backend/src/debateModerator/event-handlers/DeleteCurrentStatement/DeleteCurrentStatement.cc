#include "DeleteCurrentStatement.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void DeleteCurrentStatementHandler::DeleteCurrentStatement(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    std::vector<uint8_t> userData = debateWrapper.getUserProtobufByUsername(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());

    // get current debate and current statement
    std::string currentDebateId = userProto.engagement().debating_info().root_claim().id();
    std::string currentStatementId = userProto.engagement().debating_info().current_claim().id();

    if (currentDebateId.empty() || currentStatementId.empty()) {
        // No current debate or statement to delete
        return;
    }

    // move to parent
    std::string parentClaimId = debateWrapper.findClaimParent(currentStatementId).id();
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_claim()->set_id(parentClaimId);

    // delete the current statement from the debate
    debateWrapper.deleteClaim(currentStatementId);

    // serialize and save back
    std::vector<uint8_t> updatedUserData(userProto.ByteSizeLong());
    userProto.SerializeToArray(updatedUserData.data(), updatedUserData.size());
    debateWrapper.updateUserProtobuf(user, updatedUserData);
}