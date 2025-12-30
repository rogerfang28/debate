#pragma once

#include "../../../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../utils/DebateWrapper.h"
#include <string>

class LoginPageResponseGenerator {
public:
    static void BuildLoginPageResponse(moderator_to_vr::ModeratorToVRMessage& responseMessage);
};
