#include "DebatePageResponseGenerator.h"
#include "../../../database/handlers/DebateDatabaseHandler.h"
#include "../../../utils/pathUtils.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include <iostream>
#include <vector>

void DebatePageResponseGenerator::BuildDebatePageResponse(
    moderator_to_vr::ModeratorToVRMessage& responseMessage,
    const std::string& user,
    const user::User& userProto
) {
    // populate debate info, maybe refactor to different file later
    DebateDatabaseHandler debateDbHandler(utils::getDatabasePath());
    std::string debateID = userProto.engagement().debating_info().debate_id();
    if (debateDbHandler.debateExists(debateID)) {
        std::vector<uint8_t> debateData = debateDbHandler.getDebateProtobuf(debateID);
        debate::Debate debateProto;
        debateProto.ParseFromArray(debateData.data(), debateData.size());
        *responseMessage.mutable_debate() = debateProto;
        std::cout << "[DebateModerator] Debate Topic: " << debateProto.topic() << "\n";
        std::cout << "[DebateModerator] Added debate info to response for debate ID: " << debateID << "\n";
    }
    else{
        std::cout << "[DebateModerator] Warning: Debate ID " << debateID << " does not exist in database.\n";
    }
}
