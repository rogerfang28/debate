#include "AiAuthorHandler.h"

#include "../../DebateModerator.h"
#include "../../buildResponse/debatePageResponse/BuildCollection.h"
#include "../../../utils/AiChallenger.h"
#include "../../../utils/AiJobs.h"
#include "../../../utils/DebateGenerator.h"
#include "../../../utils/DebateTextFormatter.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../utils/ScenarioImporter.h"
#include "../../../utils/ExtractedDebateBuilder.h"
#include "../../../utils/Log.h"

#include <exception>
#include <map>
#include <string>
#include <vector>

namespace {

user_engagement::AiJobStatus* statusOf(user::User& userProto) {
    return userProto.mutable_engagement()->mutable_ai_job();
}

} // namespace

void AiAuthorHandler::GenerateDebate(const std::string& topic, const std::string& passage,
                                     const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::AiJobStatus* status = statusOf(userProto);
    status->Clear();

    if (passage.empty()) {
        status->set_error("Paste a passage for the AI to build the debate from.");
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    // The prompt is built here, on the request thread, and handed over whole.
    // The worker runs a prompt; it does not know what a debate is.
    AiJobs::Start(user_id, AiJobs::Kind::Generation, DebateGenerator::BuildPrompt(topic, passage), 0);
    status->set_pending(true);
    status->set_elapsed_seconds(0);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void AiAuthorHandler::ChallengeWithAi(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();
    user_engagement::AiJobStatus* status = statusOf(userProto);
    status->Clear();

    if (debateId <= 0) {
        status->set_error("No debate is open.");
        debateWrapper.updateUserProtobuf(user_id, userProto);
        return;
    }

    const std::vector<int> memberIds = debateWrapper.findUsersInDebate(debateId);
    const debate::Collection collection =
        BuildCollection::BuildForDebateAndUsers(debateId, memberIds, debateWrapper);
    if (collection.claims_by_id().empty()) {
        status->set_error("This debate has no claims to challenge yet.");
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
    // The same rendering the summary reads. It numbers every claim, which is
    // what lets the model point at one precisely instead of describing it.
    const std::string debateText = DebateTextFormatter::Format(collection, usernames, options);

    AiJobs::Start(user_id, AiJobs::Kind::Challenge, AiChallenger::BuildPrompt(debateText), 0);
    status->set_pending(true);
    status->set_elapsed_seconds(0);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void AiAuthorHandler::CancelWork(const int& user_id, DebateWrapper& debateWrapper) {
    // Every kind, because the button says "Cancel" and the user should not have
    // to know which of four things is currently running.
    AiJobs::Abandon(user_id, AiJobs::Kind::Generation);
    AiJobs::Abandon(user_id, AiJobs::Kind::Challenge);
    AiJobs::Abandon(user_id, AiJobs::Kind::Debate);
    AiJobs::Abandon(user_id, AiJobs::Kind::Claim);

    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::AiJobStatus* status = statusOf(userProto);
    status->Clear();
    status->set_note("Cancelled. The model may still be working, but nothing is waiting for it.");
    // Summaries keep their own pending flags, which would otherwise leave the
    // page polling for a result that will never be collected.
    user_engagement::DebatingInfo* info = userProto.mutable_engagement()->mutable_debating_info();
    info->set_summary_running(false);
    info->mutable_debate_summary()->set_pending(false);
    info->mutable_claim_summary()->set_pending(false);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void AiAuthorHandler::DismissNotice(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_engagement()->clear_ai_job();
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void AiAuthorHandler::RefreshAiWork(const int& user_id, DebateModerator& moderator) {
    DebateWrapper& debateWrapper = moderator.getDebateWrapper();

    const AiJobs::Snapshot generation = AiJobs::Collect(user_id, AiJobs::Kind::Generation);
    const AiJobs::Snapshot challenge = AiJobs::Collect(user_id, AiJobs::Kind::Challenge);
    if (!generation.running && !generation.finished && !challenge.running && !challenge.finished) {
        return;
    }

    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::AiJobStatus* status = statusOf(userProto);
    const std::string before = status->SerializeAsString();

    if (generation.running || challenge.running) {
        const AiJobs::Snapshot& live = generation.running ? generation : challenge;
        status->set_pending(true);
        status->set_elapsed_seconds(live.elapsedSeconds);
        status->clear_error();
        if (status->SerializeAsString() != before) {
            debateWrapper.updateUserProtobuf(user_id, userProto);
        }
        return;
    }

    // A finished run. Collect() hands the result over exactly once, so whatever
    // happens here is the only chance to act on it.
    if (generation.finished) {
        status->set_pending(false);
        status->set_elapsed_seconds(0);
        if (!generation.error.empty()) {
            status->set_error(generation.error);
        } else {
            // Parsing and building both write to the database, so both happen
            // HERE, on the request thread, never on the worker.
            //
            // The model now writes a TREE rather than a step list. The whole
            // tree is validated before anything is created, so a malformed
            // generation leaves no debate behind at all -- where the old step
            // list could stop halfway and leave a partial one for the user to
            // find and delete.
            const ExtractedDebateBuilder::Result built =
                ExtractedDebateBuilder::Build(generation.text, user_id, moderator);
            if (!built.ok) {
                status->set_error(built.error);
                if (built.debateId > 0) {
                    // Validation passed but a write failed, which is the one
                    // path that can still leave something behind.
                    status->set_about_debate_id(built.debateId);
                }
            } else {
                std::string note = "Generated a debate: " +
                                   std::to_string(built.claimsCreated) + " claims, " +
                                   std::to_string(built.argumentsCreated) + " arguments, " +
                                   std::to_string(built.maxDepthReached) + " levels deep.";
                if (built.groupsSplit > 0) {
                    // Worth surfacing: it says the model grouped claims it could
                    // not justify grouping, which is a fact about the generation.
                    note += " " + std::to_string(built.groupsSplit) +
                            " group(s) were split for having no stated reason to be joint.";
                }
                if (built.unknownBases > 0) {
                    note += " " + std::to_string(built.unknownBases) +
                            " argument(s) named a kind of reason that is not configured.";
                }
                status->set_note(note);
                status->set_about_debate_id(built.debateId);
            }
        }
    }

    if (challenge.finished) {
        status->set_pending(false);
        status->set_elapsed_seconds(0);
        if (!challenge.error.empty()) {
            status->set_error(challenge.error);
        } else {
            const int challengedDebateId =
                userProto.engagement().debating_info().debate_id();
            const auto applied = AiChallenger::Apply(challenge.text, user_id, moderator);
            status->set_about_debate_id(challengedDebateId);
            if (!applied.ok) {
                status->set_error(applied.error);
            } else if (applied.challengesAdded == 0) {
                status->set_note("The AI did not find anything it could challenge.");
            } else {
                std::string note = "Added " + std::to_string(applied.challengesAdded) +
                                   " AI challenge(s).";
                if (applied.skipped > 0) {
                    note += " " + std::to_string(applied.skipped) +
                            " were skipped for naming a claim that is not in this debate.";
                }
                status->set_note(note);
            }
        }
    }

    debateWrapper.updateUserProtobuf(user_id, userProto);
}
