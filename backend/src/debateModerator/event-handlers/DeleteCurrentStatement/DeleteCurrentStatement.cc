#include "DeleteCurrentStatement.h"
#include "../../../../../src/gen/cpp/user.pb.h"

void DeleteCurrentStatementHandler::DeleteCurrentStatement(const std::string& user, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobufByUsername(user);

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

    // check if its a root claim
    if (parentClaimId == "-1") {
        return;
    }

    // delete the current statement from the debate
    debateWrapper.deleteClaim(currentStatementId);

    debateWrapper.updateUserProtobuf(user, userProto);
}