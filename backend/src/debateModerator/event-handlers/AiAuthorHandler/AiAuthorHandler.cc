#include "AiAuthorHandler.h"

#include "../../DebateModerator.h"
#include "../../buildResponse/debatePageResponse/BuildCollection.h"
#include "../../../utils/AiJobs.h"
#include "../../../utils/DebateGenerator.h"
#include "../../../utils/DebateTextFormatter.h"
#include "../../../utils/DebateWrapper.h"
#include "../../../utils/ScenarioImporter.h"
#include "../../../utils/ExtractedDebateBuilder.h"
#include "../../../utils/ConnectionsDebateBuilder.h"
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

    // Two model calls: list the claims, then connect them. The second prompt
    // is built from the first answer on the worker thread; that lambda parses
    // text and formats text and touches nothing else.
    AiJobs::StartTwoStage(
        user_id, AiJobs::Kind::Generation, DebateGenerator::BuildClaimsPrompt(topic, passage),
        [topic](std::string& claimsText) {
            // When the topic is already an assertion it is the root, whatever
            // the model typed on line 1. Rewritten in place so the same pinned
            // list reaches both the second prompt and the builder.
            claimsText = ConnectionsDebateBuilder::WithRoot(
                claimsText, DebateGenerator::RootFromTopic(topic));
            const std::vector<std::string> claims =
                ConnectionsDebateBuilder::ParseClaims(claimsText);
            if (claims.empty()) {
                return std::string();
            }
            return DebateGenerator::BuildConnectionsPrompt(topic, claims);
        },
        0);
    status->set_pending(true);
    status->set_elapsed_seconds(0);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void AiAuthorHandler::CancelWork(const int& user_id, DebateWrapper& debateWrapper) {
    // Every kind, because the button says "Cancel" and the user should not have
    // to know which thing is currently running.
    AiJobs::Abandon(user_id, AiJobs::Kind::Generation);
    AiJobs::Abandon(user_id, AiJobs::Kind::Chat);

    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::AiJobStatus* status = statusOf(userProto);
    status->Clear();
    status->set_note("Cancelled. The model may still be working, but nothing is waiting for it.");
    // The chat keeps its own pending flag, which would otherwise leave the
    // page polling for an answer that will never be collected.
    userProto.mutable_engagement()->mutable_debating_info()->mutable_chat()->set_pending(false);
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
    if (!generation.running && !generation.finished) {
        return;
    }

    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    user_engagement::AiJobStatus* status = statusOf(userProto);
    const std::string before = status->SerializeAsString();

    if (generation.running) {
        const AiJobs::Snapshot& live = generation;
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
            // intermediate = the numbered claims, text = the connections.
            const ConnectionsDebateBuilder::Result built = ConnectionsDebateBuilder::Build(
                generation.intermediate, generation.text, user_id, moderator);
            if (!built.ok) {
                status->set_error(built.error);
                if (built.debateId > 0) {
                    status->set_about_debate_id(built.debateId);
                }
            } else {
                std::string note = "Generated a debate: " +
                                   std::to_string(built.claimsCreated) + " claims, " +
                                   std::to_string(built.argumentsCreated) + " arguments, " +
                                   std::to_string(built.maxDepthReached) + " levels deep.";
                // Each of these says something about the generation, not the
                // build, so the user should see them rather than a clean count.
                if (!built.dropped.empty()) {
                    note += " " + std::to_string(built.dropped.size()) +
                            " connection(s) could not be used.";
                }
                if (built.attachedToRoot > built.cyclesBroken) {
                    note += " " + std::to_string(built.attachedToRoot - built.cyclesBroken) +
                            " claim(s) were placed under the root by the builder because other "
                            "claims depended on them but the AI never said where they go.";
                }
                if (built.cyclesBroken > 0) {
                    note += " " + std::to_string(built.cyclesBroken) +
                            " claim(s) were placed under the root by the builder because the "
                            "AI's connections went round in a circle.";
                }
                if (!built.orphans.empty()) {
                    note += " " + std::to_string(built.orphans.size()) +
                            " claim(s) the AI found were left out because it did not say "
                            "where they go.";
                }
                if (built.undercuts > 0) {
                    note += " " + std::to_string(built.undercuts) +
                            " challenge(s) are aimed at an argument rather than a claim.";
                }
                if (built.groupsSplit > 0) {
                    note += " " + std::to_string(built.groupsSplit) +
                            " group(s) were split for having no stated reason to be joint.";
                }
                if (built.unknownBases > 0) {
                    note += " " + std::to_string(built.unknownBases) +
                            " connection(s) named a kind of reason that is not configured.";
                }
                for (const std::string& d : built.dropped) {
                    Log::warn("[Connections] dropped: " + d);
                }
                status->set_note(note);
                status->set_about_debate_id(built.debateId);
            }
        }
    }

    debateWrapper.updateUserProtobuf(user_id, userProto);
}
