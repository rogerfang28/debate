#include "SummarizeDebateHandler.h"

#include "../../../utils/DebateSummarizer.h"
#include "../../../utils/DebateTextFormatter.h"
#include "../../../utils/Log.h"
#include "../../buildResponse/debatePageResponse/BuildCollection.h"

#include <exception>
#include <map>
#include <string>
#include <vector>

void SummarizeDebateHandler::SummarizeDebate(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();

    user_engagement::DebatingInfo::DebateSummary* summary =
        userProto.mutable_engagement()->mutable_debating_info()->mutable_debate_summary();
    summary->Clear();

    if (debateId <= 0) {
        summary->set_error("No debate is open.");
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    // The same collection the page is drawn from, so the summary describes the
    // debate the user is actually looking at -- including statuses computed
    // from the move log rather than the stale stored ones.
    const std::vector<int> memberIds = debateWrapper.findUsersInDebate(debateId);
    const debate::Collection collection =
        BuildCollection::BuildForDebateAndUsers(debateId, memberIds, debateWrapper);

    if (collection.claims_by_id().empty()) {
        summary->set_error("This debate has no claims to summarize yet.");
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    std::map<int, std::string> usernames;
    for (int memberId : memberIds) {
        usernames[memberId] = debateWrapper.getUserProtobuf(memberId).username();
    }
    // Claim authors who are no longer members would otherwise render as
    // "User 7" in their own debate.
    for (const auto& entry : collection.claims_by_id()) {
        const int creatorId = entry.second.creator_id();
        if (creatorId > 0 && usernames.find(creatorId) == usernames.end()) {
            usernames[creatorId] = debateWrapper.getUserProtobuf(creatorId).username();
        }
    }

    std::string topic;
    const std::vector<uint8_t> debateBytes = debateWrapper.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateBytes.empty() &&
        debateProto.ParseFromArray(debateBytes.data(), static_cast<int>(debateBytes.size()))) {
        topic = debateProto.topic();
    }

    DebateTextFormatter::Options options;
    options.topic = topic;
    options.viewerUsername = userProto.username();
    const std::string debateText = DebateTextFormatter::Format(collection, usernames, options);

    // Stamped BEFORE the call: the summary describes the debate as it was when
    // the text was taken, and the call can take a minute during which anyone
    // may add a claim.
    int headSeq = 0;
    for (const debate::Move& move : debateWrapper.getMovesForDebate(debateId)) {
        if (move.seq() > headSeq) {
            headSeq = move.seq();
        }
    }
    summary->set_summarized_at_move_seq(headSeq);

    try {
        const std::string text = DebateSummarizer::Summarize(debateText);
        if (text.empty()) {
            summary->set_error("The model returned an empty summary.");
        } else {
            summary->set_text(text);
        }
    } catch (const std::exception& e) {
        // Shown to the user verbatim: when the local model is simply switched
        // off, "could not reach the model at ..." is the whole diagnosis, and
        // hiding it behind "something went wrong" helps nobody.
        Log::warn(std::string("[SummarizeDebateHandler] summary failed: ") + e.what());
        summary->set_error(e.what());
    }

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void SummarizeDebateHandler::DismissSummary(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->clear_debate_summary();
    debateWrapper.updateUserProtobuf(user_id, userProto);
}
