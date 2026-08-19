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
    SummaryJobs::Start(user_id, SummaryJobs::Kind::Debate, debateText);
    summary->set_pending(true);
    summary->set_elapsed_seconds(0);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

namespace {

// Applies one job's state to one summary field. Both summaries behave
// identically once started, so the difference between them is which field and
// which job -- not which logic.
bool applySnapshot(const SummaryJobs::Snapshot& snapshot,
                   user_engagement::DebatingInfo::DebateSummary* summary) {
    if (!snapshot.running && !snapshot.finished) {
        return false;
    }

    if (snapshot.running) {
        // Rewritten every poll so the elapsed counter actually moves.
        summary->set_pending(true);
        summary->set_elapsed_seconds(snapshot.elapsedSeconds);
        return true;
    }

    summary->set_pending(false);
    summary->set_elapsed_seconds(0);
    if (!snapshot.error.empty()) {
        summary->clear_text();
        summary->set_error(snapshot.error);
    } else {
        summary->clear_error();
        summary->set_text(snapshot.text);
    }
    return true;
}

} // namespace

void SummarizeDebateHandler::RefreshSummaryState(const int& user_id, DebateWrapper& debateWrapper) {
    const SummaryJobs::Snapshot debateSnapshot =
        SummaryJobs::Collect(user_id, SummaryJobs::Kind::Debate);
    const SummaryJobs::Snapshot claimSnapshot =
        SummaryJobs::Collect(user_id, SummaryJobs::Kind::Claim);
    if (!debateSnapshot.running && !debateSnapshot.finished &&
        !claimSnapshot.running && !claimSnapshot.finished) {
        return;
    }

    // One read, one write, whatever changed. Collect() has already handed over
    // any finished result, so skipping the write here would drop it for good.
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::DebatingInfo* info =
        userProto.mutable_engagement()->mutable_debating_info();
    bool changed = applySnapshot(debateSnapshot, info->mutable_debate_summary());
    changed = applySnapshot(claimSnapshot, info->mutable_claim_summary()) || changed;
    if (changed) {
        debateWrapper.updateUserProtobuf(user_id, userProto);
    }
}

void SummarizeDebateHandler::SummarizeClaim(const int& claim_id, const int& user_id,
                                            DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();

    user_engagement::DebatingInfo* info =
        userProto.mutable_engagement()->mutable_debating_info();
    user_engagement::DebatingInfo::DebateSummary* summary = info->mutable_claim_summary();
    summary->Clear();
    info->set_claim_summary_claim_id(claim_id);

    if (debateId <= 0 || claim_id <= 0) {
        summary->set_error("No claim is selected.");
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    const std::vector<int> memberIds = debateWrapper.findUsersInDebate(debateId);
    const debate::Collection collection =
        BuildCollection::BuildForDebateAndUsers(debateId, memberIds, debateWrapper);
    if (collection.claims_by_id().find(claim_id) == collection.claims_by_id().end()) {
        summary->set_error("That claim is not part of this debate.");
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    std::map<int, std::string> usernames;
    for (int memberId : memberIds) {
        usernames[memberId] = debateWrapper.getUserProtobuf(memberId).username();
    }
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
    const std::string contextText =
        DebateTextFormatter::FormatClaimContext(collection, usernames, options, claim_id);

    SummaryJobs::Start(user_id, SummaryJobs::Kind::Claim, contextText);
    summary->set_pending(true);
    summary->set_elapsed_seconds(0);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void SummarizeDebateHandler::DismissClaimSummary(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->clear_claim_summary();
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void SummarizeDebateHandler::DismissSummary(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->mutable_debating_info()->clear_debate_summary();
    debateWrapper.updateUserProtobuf(user_id, userProto);
}
