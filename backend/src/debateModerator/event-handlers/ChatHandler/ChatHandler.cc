#include "ChatHandler.h"

#include "../../buildResponse/debatePageResponse/BuildCollection.h"
#include "../../../utils/AiJobs.h"
#include "../../../utils/DebateChat.h"
#include "../../../utils/DebateTextFormatter.h"
#include "../../../utils/LlmClient.h"
#include "../../../utils/Log.h"
#include "../../../utils/UsernamePolicy.h"

#include <map>
#include <string>
#include <vector>

namespace {

// How many messages ride on the user record, and how many go to the model.
// The table keeps everything; these are what the page and the prompt see.
constexpr int kKeptOnRecord = 30;
constexpr int kSentToModel = 10;

user_engagement::DebatingInfo::ChatState* stateOf(user::User& u) {
    return u.mutable_engagement()->mutable_debating_info()->mutable_chat();
}

void pushMessage(user_engagement::DebatingInfo::ChatState* chat, const std::string& role,
                 const std::string& text) {
    user_engagement::DebatingInfo::ChatMessage* m = chat->add_messages();
    m->set_role(role);
    m->set_text(text);
    while (chat->messages_size() > kKeptOnRecord) {
        chat->mutable_messages()->DeleteSubrange(0, 1);
    }
}

// Reloads the record's copy from the table. Called when the record is for a
// different debate than the one the user is in, or has never been filled.
void loadTranscript(user_engagement::DebatingInfo::ChatState* chat, int user_id, int debateId,
                    DebateWrapper& debateWrapper) {
    chat->clear_messages();
    for (const auto& m : debateWrapper.chatTranscript(user_id, debateId)) {
        pushMessage(chat, m.role, m.text);
    }
    chat->set_debate_id(debateId);
}

std::string trimmed(const std::string& s) {
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    const size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

}  // namespace

void ChatHandler::Ask(const std::string& question, const std::string& quickPrompt,
                      const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();
    if (debateId <= 0) {
        Log::warn("[ChatHandler] User " + std::to_string(user_id) + " asked with no debate open");
        return;
    }
    if (AiJobs::IsRunning(user_id, AiJobs::Kind::Chat)) {
        Log::info("[ChatHandler] User " + std::to_string(user_id) +
                  " already has an answer coming; ignoring the new question");
        return;
    }

    // The same collection the page is drawn from, so the model reads the
    // debate the reader is looking at, statuses included.
    const std::vector<int> memberIds = debateWrapper.findUsersInDebate(debateId);
    const debate::Collection collection =
        BuildCollection::BuildForDebateAndUsers(debateId, memberIds, debateWrapper);

    std::map<int, std::string> usernames;
    for (int memberId : memberIds) {
        usernames[memberId] = debateWrapper.getUserProtobuf(memberId).username();
    }
    for (const auto& entry : collection.claims_by_id()) {
        for (const int who : {entry.second.creator_id(), entry.second.voice_id()}) {
            if (who > 0 && usernames.find(who) == usernames.end()) {
                usernames[who] = debateWrapper.getUserProtobuf(who).username();
            }
        }
    }

    std::string topic;
    const std::vector<uint8_t> debateBytes = debateWrapper.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateBytes.empty() &&
        debateProto.ParseFromArray(debateBytes.data(), static_cast<int>(debateBytes.size()))) {
        topic = debateProto.topic();
    }

    // What is being asked: the typed question, or the quick prompt's.
    std::string asked = trimmed(question);
    if (!quickPrompt.empty()) {
        const int claimId = userProto.engagement().debating_info().current_claim().id();
        std::string sentence;
        auto it = collection.claims_by_id().find(claimId);
        if (it != collection.claims_by_id().end()) sentence = it->second.sentence();
        asked = DebateChat::QuestionFor(quickPrompt, claimId, sentence);
    }
    if (asked.empty()) {
        return;
    }

    user_engagement::DebatingInfo::ChatState* chat = stateOf(userProto);
    if (chat->debate_id() != debateId) {
        loadTranscript(chat, user_id, debateId, debateWrapper);
    }
    debateWrapper.appendChatMessage(user_id, debateId, "user", asked);
    pushMessage(chat, "user", asked);

    DebateTextFormatter::Options options;
    options.topic = topic;
    options.viewerUsername = userProto.username();
    // A closed account is "Deleted account" to the model too.
    const std::string debateText =
        username_policy::ScrubDeletedNames(DebateTextFormatter::Format(collection, usernames, options));

    std::vector<LlmMessage> messages;
    messages.push_back({"system", DebateChat::SystemPrompt(debateText, userProto.username())});
    const int total = chat->messages_size();
    for (int i = total > kSentToModel ? total - kSentToModel : 0; i < total; ++i) {
        const auto& m = chat->messages(i);
        messages.push_back({m.role() == "assistant" ? "assistant" : "user", m.text()});
    }

    // Hand off and return; the answer arrives through Refresh.
    AiJobs::StartChat(user_id, messages);
    chat->set_pending(true);
    chat->set_elapsed_seconds(0);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void ChatHandler::Refresh(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();
    user_engagement::DebatingInfo::ChatState* chat = stateOf(userProto);
    const std::string before = chat->SerializeAsString();

    if (debateId > 0 && chat->debate_id() != debateId) {
        loadTranscript(chat, user_id, debateId, debateWrapper);
    }

    const AiJobs::Snapshot answer = AiJobs::Collect(user_id, AiJobs::Kind::Chat);
    if (answer.running) {
        chat->set_pending(true);
        chat->set_elapsed_seconds(answer.elapsedSeconds);
    } else if (answer.finished) {
        const std::string text = !answer.error.empty()
            ? "Sorry, I could not answer: " + answer.error
            : answer.text;
        if (debateId > 0) {
            debateWrapper.appendChatMessage(user_id, debateId, "assistant", text);
        }
        pushMessage(chat, "assistant", text);
        chat->set_pending(false);
        chat->set_elapsed_seconds(0);
    } else if (chat->pending()) {
        // Pending with nothing running: the server restarted mid-answer.
        chat->set_pending(false);
        chat->set_elapsed_seconds(0);
    }

    if (chat->SerializeAsString() != before) {
        debateWrapper.updateUserProtobuf(user_id, userProto);
    }
}

void ChatHandler::Clear(const int& user_id, DebateWrapper& debateWrapper) {
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    const int debateId = userProto.engagement().debating_info().debate_id();
    if (debateId > 0) {
        debateWrapper.clearChat(user_id, debateId);
    }
    user_engagement::DebatingInfo::ChatState* chat = stateOf(userProto);
    chat->clear_messages();
    chat->set_debate_id(debateId);
    debateWrapper.updateUserProtobuf(user_id, userProto);
}
