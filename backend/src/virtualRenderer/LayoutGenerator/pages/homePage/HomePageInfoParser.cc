#include "HomePageInfoParser.h"
#include "../../../../utils/AnonymousUser.h"
#include "../../../../utils/Log.h"

rendering_info::HomePageRenderingInfo HomePageInfoParser::ParseFromResponse(const moderator_to_vr::ModeratorToVRMessage& responseMessage) {
    rendering_info::HomePageRenderingInfo info;
    const user::User& userProto = responseMessage.user();
    const moderator_to_vr::DebateList& debateList = responseMessage.debate_list();

    // Who may delete which debate: its creator.
    info.set_viewer_user_id(userProto.user_id());
    info.set_viewer_username(userProto.username());
    // A visitor sees create / join / walkthrough too: pressing one opens the
    // sign-in modal, and the press is replayed once they have signed in.
    info.set_can_create_or_join_debates(!anonymous_user::IsReadOnlyViewer(userProto.username()));
    // Carried through so the page can show a generation that is still running,
    // or what the last one produced.
    *info.mutable_ai_job() = userProto.engagement().ai_job();

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
        outTopic->set_root_text(topic.root_text());
        outTopic->set_root_holder(topic.root_holder());
        outTopic->set_unresolved(topic.unresolved());
        outTopic->set_hinge_text(topic.hinge_text());
        outTopic->set_claim_count(topic.claim_count());
        outTopic->set_stance_count(topic.stance_count());
        outTopic->set_open_challenge_count(topic.open_challenge_count());
        *outTopic->mutable_involved() = topic.involved();
        outTopic->set_last_moved_at(topic.last_moved_at());
        outTopic->set_people_count(topic.people_count());
        *outTopic->mutable_owing() = topic.owing();
        // Only what the viewer owes: "Waiting on you".
        for (const auto& owed : topic.owed()) {
            if (!userProto.username().empty() && owed.who() == userProto.username()) {
                *outTopic->add_owed_to_viewer() = owed;
            }
        }
        for (const int member : topic.member_ids()) {
            if (member == userProto.user_id()) outTopic->set_viewer_is_member(true);
        }
        *outTopic->mutable_recent_moves() = topic.recent_moves();
        outTopic->set_search_match(topic.search_match());
        *outTopic->mutable_matched_claims() = topic.matched_claims();
    }
    *info.mutable_home_view() = userProto.engagement().home_view();
    if (responseMessage.has_profile()) {
        *info.mutable_profile() = responseMessage.profile();
    }

    return info;
}
