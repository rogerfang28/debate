#include "HomePageInfoParser.h"
#include "../../../../utils/DemoMode.h"

rendering_info::HomePageRenderingInfo HomePageInfoParser::ParseFromUser(const user::User& userProto) {
    rendering_info::HomePageRenderingInfo info;

    info.set_viewer_user_id(userProto.user_id());
    info.set_viewer_username(userProto.username());
    info.set_can_create_or_join_debates(!demo_mode::kViewerModeEnabled);

    const user_engagement::DebateList& debateList = userProto.engagement().home_info().available_debates();
    for (const auto& topic : debateList.topics()) {
        rendering_info::HomeDebateTopicRenderInfo* outTopic = info.add_available_debates();
        outTopic->set_id(topic.id());
        outTopic->set_topic(topic.topic());
        outTopic->set_creator_id(topic.creator_id());
        outTopic->set_is_challenge(topic.is_challenge());
        outTopic->set_claim_its_challenging(topic.claim_its_challenging());
    }

    return info;
}
