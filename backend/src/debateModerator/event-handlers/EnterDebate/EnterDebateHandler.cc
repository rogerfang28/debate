#include "EnterDebateHandler.h"
// #include "../../../database/handlers/DebateDatabaseHandler.h"
// #include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"
#include <iostream>
#include <vector>
#include "../../../utils/Log.h"

bool EnterDebateHandler::EnterDebate(const std::string& debateId, const std::string& user, DebateWrapper& debateWrapper) {
    Log::debug("[EnterDebate] User " + user + " entering debate with id: " + debateId);

    DebateDatabaseHandler debateDbHandler(utils::getDatabasePath());
    std::vector<uint8_t> debateData = debateDbHandler.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateProto.ParseFromArray(debateData.data(), static_cast<int>(debateData.size()))) {
        Log::error("[EnterDebate][ERR] Failed to parse debate protobuf for debate ID " + debateId);
        return false;
    }
    std::string rootClaimId = debateProto.root_claim_id();

    debateWrapper.moveUserToClaim(user, rootClaimId);
    return true;
}
