#include "SummarizeDebateHandler.h"

#include "../../../utils/SummaryJobs.h"
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

    // Hand off and return. Blocking here would hold the request thread for the
    // whole model call -- tens of seconds to minutes on a real debate -- so the
    // page would not come back until it finished and the user would be staring
    // at a dead tab with no way to tell progress from a hang.
    SummaryJobs::Start(user_id, debateText);
    summary->set_pending(true);
    summary->set_elapsed_seconds(0);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void SummarizeDebateHandler::RefreshSummaryState(const int& user_id, DebateWrapper& debateWrapper) {
    const SummaryJobs::Snapshot snapshot = SummaryJobs::Collect(user_id);
    if (!snapshot.running && !snapshot.finished) {
        return;
    }

    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::DebatingInfo::DebateSummary* summary =
        userProto.mutable_engagement()->mutable_debating_info()->mutable_debate_summary();

    if (snapshot.running) {
        // Rewritten every poll so the elapsed counter actually moves.
        summary->set_pending(true);
        summary->set_elapsed_seconds(snapshot.elapsedSeconds);
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    // Finished. This runs on the request thread, which is the only thread that
    // touches the database -- the worker never does.
    summary->set_pending(false);
    summary->set_elapsed_seconds(0);
    if (!snapshot.error.empty()) {
        summary->clear_text();
        summary->set_error(snapshot.error);
    } else {
        summary->clear_error();
        summary->set_text(snapshot.text);
    }
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void SummarizeDebateHandler::DismissSummary(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->clear_debate_summary();
    debateWrapper.updateUserProtobuf(user_id, userProto);
}
