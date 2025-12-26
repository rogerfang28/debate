#ifndef DEBATEPAGERESPONSEGENERATOR_H
#define DEBATEPAGERESPONSEGENERATOR_H

#include "../../../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include <string>
#include "../../../utils/DebateWrapper.h"

class DebatePageResponseGenerator {
public:
    static void BuildDebatePageResponse(
        moderator_to_vr::ModeratorToVRMessage& responseMessage,
        const std::string& user,
        const user::User& userProto,
        DebateWrapper& debateWrapper
    );
};

#endif // DEBATEPAGERESPONSEGENERATOR_H
