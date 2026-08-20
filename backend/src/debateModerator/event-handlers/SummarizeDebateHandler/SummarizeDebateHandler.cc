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
    // Hand off and return. Blocking here would hold the request thread for the
    // whole model call -- tens of seconds to minutes on a real debate -- so the
    // page would not come back until it finished and the user would be staring
    // at a dead tab with no way to tell progress from a hang.
    SummaryJobs::Start(user_id, SummaryJobs::Kind::Debate, debateText, headSeq);
    summary->set_pending(true);
    summary->set_elapsed_seconds(0);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

namespace {

constexpr const char* kDebateKind = "debate";
constexpr const char* kClaimKind = "claim";

// Overlays one job's state onto the response's summary field.
//
// Pending state is deliberately NOT persisted. It belongs to a thread in this
// process, so a stored "pending" would outlive the run that justified it; and
// writing the elapsed counter every poll meant rewriting the whole user record
// every two seconds. Only a FINISHED summary reaches the database.
void applyRunningState(const SummaryJobs::Snapshot& snapshot,
                       user_engagement::DebatingInfo::DebateSummary* summary) {
    summary->set_pending(true);
    summary->set_elapsed_seconds(snapshot.elapsedSeconds);
}

void applyStored(const SummaryDatabase::Record& record, int headSeq,
                 user_engagement::DebatingInfo::DebateSummary* summary) {
    if (!record.found) {
        return;
    }
    summary->set_pending(false);
    summary->set_elapsed_seconds(0);
    summary->set_summarized_at_move_seq(record.atMoveSeq);
    // Reported rather than hidden: the debate keeps moving, and a summary that
    // silently describes an older state is worse than one that admits it.
    summary->set_moves_behind(headSeq > record.atMoveSeq ? headSeq - record.atMoveSeq : 0);
    if (!record.error.empty()) {
        summary->set_error(record.error);
    } else {
        summary->set_text(record.text);
    }
}

} // namespace

void SummarizeDebateHandler::RefreshSummaryState(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::DebatingInfo* info =
        userProto.mutable_engagement()->mutable_debating_info();
    const int debateId = info->debate_id();
    if (debateId <= 0) {
        return;
    }

    // What the summary fields looked like coming in. Compared at the end so
    // the user record is only rewritten when something actually changed --
    // this runs on EVERY response, including a poll every two seconds, and
    // writing the whole record each time is the amplification that moving
    // summaries into their own table was meant to remove.
    const std::string before = info->SerializeAsString();

    // Collect() hands a finished result over exactly once, so it must be
    // written now or it is lost.
    const SummaryJobs::Snapshot debateSnapshot =
        SummaryJobs::Collect(user_id, SummaryJobs::Kind::Debate);
    const SummaryJobs::Snapshot claimSnapshot =
        SummaryJobs::Collect(user_id, SummaryJobs::Kind::Claim);
    // Two different claims, deliberately. A finished result belongs to the
    // claim its run was started for; what the page should SHOW is a summary of
    // the claim currently selected. Conflating them meant that summarizing a
    // second claim hid the first -- the row was still in the table, nothing
    // ever looked it up again.
    const int selectedClaimId = info->current_claim().id();

    if (debateSnapshot.finished) {
        debateWrapper.saveSummary(user_id, debateId, kDebateKind, 0, debateSnapshot.text,
                                  debateSnapshot.error, debateSnapshot.atMoveSeq);
    }
    if (claimSnapshot.finished) {
        debateWrapper.saveSummary(user_id, debateId, kClaimKind, claimSnapshot.claimId,
                                  claimSnapshot.text, claimSnapshot.error, claimSnapshot.atMoveSeq);
    }

    // Tracks the WORK, not a panel. What the client polls on has to outlive
    // whichever summary happens to finish first, or a concurrent run is left
    // with nobody to collect it.
    info->set_summary_running(debateSnapshot.running || claimSnapshot.running);

    // Rebuild the response's view from the store plus whatever is running. The
    // user record itself now carries no summary text at all.
    const int headSeq = debateWrapper.headMoveSeq(debateId);
    info->clear_debate_summary();
    info->clear_claim_summary();

    if (debateSnapshot.running) {
        applyRunningState(debateSnapshot, info->mutable_debate_summary());
    } else {
        applyStored(debateWrapper.loadSummary(user_id, debateId, kDebateKind, 0), headSeq,
                    info->mutable_debate_summary());
    }

    if (selectedClaimId > 0) {
        info->set_claim_summary_claim_id(selectedClaimId);
        // A run for a DIFFERENT claim must not show as progress on this one.
        if (claimSnapshot.running && claimSnapshot.claimId == selectedClaimId) {
            applyRunningState(claimSnapshot, info->mutable_claim_summary());
        } else {
            applyStored(debateWrapper.loadSummary(user_id, debateId, kClaimKind, selectedClaimId),
                        headSeq, info->mutable_claim_summary());
        }
    }

    if (info->SerializeAsString() != before) {
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

    SummaryJobs::Start(user_id, SummaryJobs::Kind::Claim, contextText,
                       debateWrapper.headMoveSeq(debateId), claim_id);
    summary->set_pending(true);
    summary->set_elapsed_seconds(0);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void SummarizeDebateHandler::DismissClaimSummary(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::DebatingInfo* info = userProto.mutable_engagement()->mutable_debating_info();
    debateWrapper.deleteSummary(user_id, info->debate_id(), "claim", info->current_claim().id());
    info->clear_claim_summary();
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void SummarizeDebateHandler::DismissSummary(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();
    debateWrapper.deleteSummary(user_id, debateId, "debate", 0);
    userProto.mutable_engagement()->mutable_debating_info()->clear_debate_summary();
    debateWrapper.updateUserProtobuf(user_id, userProto);
}
