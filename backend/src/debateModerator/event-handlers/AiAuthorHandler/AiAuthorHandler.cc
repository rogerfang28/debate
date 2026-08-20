#include "AiAuthorHandler.h"

#include "../../DebateModerator.h"
#include "../../buildResponse/debatePageResponse/BuildCollection.h"
#include "../../../utils/AiChallenger.h"
#include "../../../utils/AiJobs.h"
#include "../../../utils/DebateGenerator.h"
#include "../../../utils/DebateTextFormatter.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../utils/GeneratedDebateBuilder.h"
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
            // The build writes to the database, so it happens HERE, on the
            // request thread, never on the worker.
            const auto built = GeneratedDebateBuilder::Build(generation.text, user_id, moderator);
            if (!built.ok) {
                status->set_error(built.error);
            } else {
                std::string note = "Generated a debate with " +
                                   std::to_string(built.claimsCreated) + " claims.";
                if (built.groupsSplit > 0) {
                    // Said out loud rather than silently corrected: the user is
                    // entitled to know the AI grouped claims it could not
                    // justify, and that the tool took the cautious reading.
                    note += " " + std::to_string(built.groupsSplit) +
                            " group(s) claimed to be joint without saying why, so they were "
                            "kept as separate arguments.";
                }
                status->set_note(note);
            }
        }
    }

    if (challenge.finished) {
        status->set_pending(false);
        status->set_elapsed_seconds(0);
        if (!challenge.error.empty()) {
            status->set_error(challenge.error);
        } else {
            const auto applied = AiChallenger::Apply(challenge.text, user_id, moderator);
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
