#ifndef HOMEPAGERESPONSEGENERATOR_H
#define HOMEPAGERESPONSEGENERATOR_H

#include "../../../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../utils/DebateWrapper.h"
#include <string>

class HomePageResponseGenerator {
public:
    static void BuildHomePageResponse(
        moderator_to_vr::ModeratorToVRMessage& responseMessage,
        const std::string& user,
        DebateWrapper& debateWrapper
    );
};

#endif // HOMEPAGERESPONSEGENERATOR_H
