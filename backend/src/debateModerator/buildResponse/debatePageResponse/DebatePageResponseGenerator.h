#ifndef DEBATEPAGERESPONSEGENERATOR_H
#define DEBATEPAGERESPONSEGENERATOR_H

#include "moderator_to_vr.pb.h"
#include "user.pb.h"
#include <string>
#include "../../../utils/DebateWrapper.h"

class DebatePageResponseGenerator {
public:
    static void BuildDebatePageResponse(
        moderator_to_vr::ModeratorToVRMessage& responseMessage,
        const int& user_id,
        const user::User& userProto,
        DebateWrapper& debateWrapper
    );
};

#endif // DEBATEPAGERESPONSEGENERATOR_H
