#ifndef DEBATEPAGERESPONSEGENERATOR_H
#define DEBATEPAGERESPONSEGENERATOR_H

#include "../../../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include <string>

class DebatePageResponseGenerator {
public:
    static void BuildDebatePageResponse(
        moderator_to_vr::ModeratorToVRMessage& responseMessage,
        const std::string& user,
        const user::User& userProto
    );
};

#endif // DEBATEPAGERESPONSEGENERATOR_H
