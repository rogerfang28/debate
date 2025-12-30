#include "LoginPageResponseGenerator.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"

void LoginPageResponseGenerator::BuildLoginPageResponse(moderator_to_vr::ModeratorToVRMessage& responseMessage) {
    responseMessage.mutable_engagement()->set_current_action(user_engagement::ACTION_LOGIN);
}
