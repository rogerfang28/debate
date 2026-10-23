#include "BriefHandler.h"

#include "../MoveUserHandler/MoveUserHandler.h"
#include "../../buildResponse/debatePageResponse/BuildCollection.h"
#include "../../../utils/AiJobs.h"
#include "../../../utils/DebateBrief.h"
#include "../../../utils/Log.h"
#include "../../../utils/UsernamePolicy.h"

#include <map>
#include <string>
#include <vector>

namespace {

user_engagement::DebatingInfo::BriefState* stateOf(user::User& u) {
    return u.mutable_engagement()->mutable_debating_info()->mutable_brief();
}

// Copies the stored words for `debateId` onto the record, or clears the
// record's copy when there are none. Returns true if anything changed.
void mirrorStored(user_engagement::DebatingInfo::BriefState* brief, int debateId,
                  DebateWrapper& debateWrapper) {
    std::string fingerprint;
    std::string text;
    if (debateId > 0 && debateWrapper.briefFor(debateId, &fingerprint, &text)) {
        if (brief->prose_fingerprint() != fingerprint || brief->prose_json() != text) {
            brief->set_prose_fingerprint(fingerprint);
            brief->set_prose_json(text);
        }
    } else if (!brief->prose_json().empty() || !brief->prose_fingerprint().empty()) {
        brief->clear_prose_json();
        brief->clear_prose_fingerprint();
    }
}

}  // namespace

void BriefHandler::Open(const int& user_id, DebateWrapper& debateWrapper) {
    // The Debate tab's own navigation, so anything half-done (a challenge
    // being assembled, say) is dropped the same way; then the flag.
    MoveUserHandler::GoToOverview(user_id, debateWrapper);
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    userProto.mutable_current_scope()->mutable_full_debate()->set_brief_view(true);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void BriefHandler::OpenClaimInDebate(const int& claim_id, const int& user_id,
                                     DebateWrapper& debateWrapper) {
    MoveUserHandler::GoToOverview(user_id, debateWrapper);
    if (claim_id > 0) {
        MoveUserHandler::SelectClaim(claim_id, user_id, debateWrapper);
    }
}

void BriefHandler::Generate(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();
    if (debateId <= 0) {
        Log::warn("[BriefHandler] User " + std::to_string(user_id) + " asked for a brief with no debate open");
        return;
    }
    if (AiJobs::IsRunning(user_id, AiJobs::Kind::Brief)) {
        Log::info("[BriefHandler] User " + std::to_string(user_id) + " already has a brief coming");
        return;
    }

    // The same collection the page is drawn from, names included.
    const std::vector<int> memberIds = debateWrapper.findUsersInDebate(debateId);
    const debate::Collection collection =
        BuildCollection::BuildForDebateAndUsers(debateId, memberIds, debateWrapper);
    std::map<int, std::string> usernames;
    auto nameOf = [&](int id) -> std::string {
        if (id <= 0) return std::string();
        auto it = usernames.find(id);
        if (it != usernames.end()) return it->second;
        const std::string name = debateWrapper.getUserProtobuf(id).username();
        usernames[id] = name;
        return name;
    };

    std::string topic;
    const std::vector<uint8_t> debateBytes = debateWrapper.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateBytes.empty() &&
        debateProto.ParseFromArray(debateBytes.data(), static_cast<int>(debateBytes.size()))) {
        topic = debateProto.topic();
    }

    const DebateBrief::Facts facts = DebateBrief::Compute(collection, nameOf);
    // A closed account is "Deleted account" to the model, so its words never name one.
    const std::string prompt = username_policy::ScrubDeletedNames(DebateBrief::Prompt(facts, topic));

    if (!AiJobs::Start(user_id, AiJobs::Kind::Brief, prompt, 0, 0)) {
        return;
    }
    user_engagement::DebatingInfo::BriefState* brief = stateOf(userProto);
    brief->set_pending(true);
    brief->set_elapsed_seconds(0);
    brief->clear_error();
    brief->set_debate_id(debateId);
    brief->set_fingerprint(DebateBrief::Fingerprint(facts));
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void BriefHandler::Refresh(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();
    user_engagement::DebatingInfo::BriefState* brief = stateOf(userProto);
    const std::string before = brief->SerializeAsString();

    const AiJobs::Snapshot run = AiJobs::Collect(user_id, AiJobs::Kind::Brief);
    if (run.running) {
        brief->set_pending(true);
        brief->set_elapsed_seconds(run.elapsedSeconds);
    } else if (run.finished) {
        brief->set_pending(false);
        brief->set_elapsed_seconds(0);
        if (!run.error.empty()) {
            brief->set_error("The brief could not be written: " + run.error);
        } else {
            DebateBrief::Prose prose;
            std::string parseError;
            if (!DebateBrief::ParseProse(run.text, &prose, &parseError)) {
                brief->set_error("The brief could not be written: " + parseError);
            } else {
                brief->clear_error();
                debateWrapper.saveBrief(brief->debate_id(), brief->fingerprint(),
                                        DebateBrief::SerializeProse(prose));
            }
        }
    } else if (brief->pending()) {
        // Pending with nothing running: the server restarted mid-run.
        brief->set_pending(false);
        brief->set_elapsed_seconds(0);
    }

    // The record's copy of the words, for the debate the user is in.
    mirrorStored(brief, debateId, debateWrapper);

    if (brief->SerializeAsString() != before) {
        debateWrapper.updateUserProtobuf(user_id, userProto);
    }
}
