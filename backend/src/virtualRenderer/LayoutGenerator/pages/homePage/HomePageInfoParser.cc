#include "HomePageInfoParser.h"
#include "../../../../utils/DemoMode.h"
#include "../../../../utils/Log.h"

rendering_info::HomePageRenderingInfo HomePageInfoParser::ParseFromResponse(const moderator_to_vr::ModeratorToVRMessage& responseMessage) {
    rendering_info::HomePageRenderingInfo info;
    const user::User& userProto = responseMessage.user();
    const moderator_to_vr::DebateList& debateList = responseMessage.debate_list();

    info.set_viewer_user_id(userProto.user_id());
    info.set_viewer_username(userProto.username());
    info.set_can_create_or_join_debates(!demo_mode::kViewerModeEnabled);

    Log::debug(
        "[HomePageInfoParser] Parsing home debate list for user_id=" + std::to_string(userProto.user_id()) +
        ", topic_count=" + std::to_string(debateList.topics_size())
    );

    for (const auto& topic : debateList.topics()) {
        Log::debug(
            "[HomePageInfoParser] topic root_claim_id=" + std::to_string(topic.id()) +
            ", topic=\"" + topic.topic() + "\"" +
            ", creator_id=" + std::to_string(topic.creator_id()) +
            ", is_challenge=" + std::string(topic.is_challenge() ? "true" : "false")
        );

        rendering_info::HomeDebateTopicRenderInfo* outTopic = info.add_available_debates();
        outTopic->set_debate_root_claim_id(topic.id());
        outTopic->set_topic(topic.topic());
        outTopic->set_creator_id(topic.creator_id());
        outTopic->set_is_challenge(topic.is_challenge());
        outTopic->set_claim_its_challenging(topic.claim_its_challenging());
    }

    return info;
}
