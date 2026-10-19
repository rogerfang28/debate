// test_chat.cc -- the AI chat: storage, the canned questions, and the flow
// from a question to an answer on the user record.
//
// The model is never reached: the fixture points LlmClient at a closed port,
// so a run fails fast and the failure arrives the way an answer would. That
// is the path worth pinning -- the plumbing, not the prose.

#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>

#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "debateModerator/DebateModerator.h"
#include "debateModerator/buildResponse/debatePageResponse/BuildCollection.h"
#include "utils/DebateChat.h"
#include "utils/DebateWrapper.h"
#include "virtualRenderer/LayoutGenerator/pages/debatePage/StepView/StepView.h"
#include "virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView/FullDebatePageInfoParser.h"
#include "database/virtualrenderer/VRUserDatabase.h"
#include "debate_event.pb.h"

#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

namespace {

void setEnv(const char* name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    if (value.empty()) unsetenv(name); else setenv(name, value.c_str(), 1);
#endif
}

}  // namespace

// ---------------------------------------------------------------------------
// Storage
// ---------------------------------------------------------------------------
class ChatStoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = std::string("test_chatdb_") + std::to_string(GETPID()) + ".sqlite3";
        std::remove(db_path_.c_str());
        db_ = new Database(db_path_);
        wrapper_ = new DatabaseWrapper(*db_, *db_);
        ASSERT_TRUE(wrapper_->ensureAllTables());
    }
    void TearDown() override {
        delete wrapper_;
        delete db_;
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
    }
    ChatDatabase& store() { return wrapper_->chats; }

    std::string db_path_;
    Database* db_ = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
};

TEST_F(ChatStoreTest, MessagesComeBackInOrderForTheirOwnerOnly) {
    ASSERT_TRUE(store().append(5, 20, "user", "first"));
    ASSERT_TRUE(store().append(5, 20, "assistant", "second"));
    ASSERT_TRUE(store().append(6, 20, "user", "someone else"));
    ASSERT_TRUE(store().append(5, 21, "user", "another debate"));

    const auto mine = store().list(5, 20);
    ASSERT_EQ(mine.size(), 2u);
    EXPECT_EQ(mine[0].role, "user");
    EXPECT_EQ(mine[0].text, "first");
    EXPECT_EQ(mine[1].role, "assistant");
    EXPECT_EQ(mine[1].text, "second");
    EXPECT_EQ(store().list(6, 20).size(), 1u);
    EXPECT_EQ(store().list(5, 21).size(), 1u);
}

TEST_F(ChatStoreTest, ClearForgetsOneConversationAndNoOther) {
    store().append(5, 20, "user", "a");
    store().append(5, 21, "user", "b");
    ASSERT_TRUE(store().clear(5, 20));
    EXPECT_TRUE(store().list(5, 20).empty());
    EXPECT_EQ(store().list(5, 21).size(), 1u);
}

TEST_F(ChatStoreTest, RefusesAMessageWithNoOwnerOrDebate) {
    EXPECT_FALSE(store().append(0, 20, "user", "x"));
    EXPECT_FALSE(store().append(5, 0, "user", "x"));
}

// ---------------------------------------------------------------------------
// The canned questions
// ---------------------------------------------------------------------------
TEST(DebateChatPrompts, QuickPromptsNameTheClaimAndUnknownKeysAreEmpty) {
    EXPECT_NE(DebateChat::QuestionFor(DebateChat::kSummarizeDebate, 0, "").find("Summarize"),
              std::string::npos);
    EXPECT_NE(DebateChat::QuestionFor(DebateChat::kFindMistake, 0, "").find("weakest"),
              std::string::npos);
    const std::string claim = DebateChat::QuestionFor(DebateChat::kSummarizeClaim, 7, "The bridge is unsafe");
    EXPECT_NE(claim.find("[#7]"), std::string::npos);
    EXPECT_NE(claim.find("The bridge is unsafe"), std::string::npos);
    EXPECT_NE(DebateChat::QuestionFor(DebateChat::kSuggestChallenge, 7, "x").find("[#7]"),
              std::string::npos);
    EXPECT_EQ(DebateChat::QuestionFor(DebateChat::kSummarizeClaim, 0, ""), "")
        << "a claim prompt with no claim selected asks nothing";
    EXPECT_EQ(DebateChat::QuestionFor("nonsense", 7, "x"), "");
    EXPECT_EQ(DebateChat::LabelFor("nonsense"), "");
}

TEST(DebateChatPrompts, TheSystemPromptCarriesTheRulesAndTheDebate) {
    const std::string p = DebateChat::SystemPrompt("- [#1] the root", "alice");
    EXPECT_NE(p.find("NEVER INVENT OPPOSITION"), std::string::npos);
    EXPECT_NE(p.find("- [#1] the root"), std::string::npos);
    EXPECT_NE(p.find("alice"), std::string::npos);
    EXPECT_NE(p.find("No outside facts"), std::string::npos);
}

// ---------------------------------------------------------------------------
// The flow, through the moderator
// ---------------------------------------------------------------------------
class ChatFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = std::string("test_chatflow_") + std::to_string(GETPID()) + ".sqlite3";
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
        setEnv("DB_PATH", db_path_);
        // A closed port: the model call fails at once, and the failure comes
        // back through the same path an answer would.
        setEnv("DEBATE_LLM_HOST", "127.0.0.1");
        setEnv("DEBATE_LLM_PORT", "9");
        setEnv("DEBATE_LLM_TIMEOUT_SEC", "2");

        db_ = new Database(db_path_);
        moderator_ = new DebateModerator(*db_);
        wrapper_ = new DatabaseWrapper(*db_, *db_);
        ASSERT_TRUE(wrapper_->ensureAllTables());
        aliceId_ = moderator_->createUserIfNotExist("alice");
        ASSERT_GT(aliceId_, 0);
    }
    void TearDown() override {
        delete wrapper_;
        delete moderator_;
        delete db_;
        for (const char* name : {"DB_PATH", "DEBATE_LLM_HOST", "DEBATE_LLM_PORT", "DEBATE_LLM_TIMEOUT_SEC"}) {
            setEnv(name, "");
        }
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
    }

    debate_event::DebateEvent event(debate_event::EventType type) {
        debate_event::DebateEvent e;
        e.mutable_user()->set_user_id(aliceId_);
        e.mutable_user()->set_username("alice");
        e.mutable_user()->set_is_logged_in(true);
        e.set_type(type);
        return e;
    }
    void send(debate_event::DebateEvent& e) { moderator_->handleRequest(e); }
    DebateWrapper& dw() { return moderator_->getDebateWrapper(); }
    const user_engagement::DebatingInfo::ChatState& chat() {
        user_ = dw().getUserProtobuf(aliceId_);
        return user_.engagement().debating_info().chat();
    }

    int createAndEnterDebate(const std::string& topic) {
        auto create = event(debate_event::CREATE_DEBATE);
        create.mutable_create_debate()->set_debate_topic(topic);
        send(create);
        const std::vector<int> ids = dw().getUserDebateIds(aliceId_);
        const int debateId = ids.empty() ? 0 : ids.back();
        auto enter = event(debate_event::ENTER_DEBATE);
        enter.mutable_enter_debate()->set_debate_id(debateId);
        send(enter);
        return debateId;
    }
    // Any request runs Refresh; a harmless one is going to the root.
    void poke() {
        auto e = event(debate_event::GO_TO_CLAIM);
        e.mutable_go_to_claim()->set_claim_id(
            dw().getUserProtobuf(aliceId_).engagement().debating_info().root_claim().id());
        send(e);
    }
    // Pokes until the answer has landed or `seconds` have passed.
    bool waitForAnswer(int seconds) {
        for (int i = 0; i < seconds * 4; ++i) {
            poke();
            if (!chat().pending()) return true;
            std::this_thread::sleep_for(std::chrono::milliseconds(250));
        }
        return false;
    }
    std::string renderPage() {
        const int debateId = dw().getUserProtobuf(aliceId_).engagement().debating_info().debate_id();
        const debate::Collection c = BuildCollection::BuildForDebateAndUsers(
            debateId, dw().findUsersInDebate(debateId), dw());
        const int claimId = dw().getUserProtobuf(aliceId_).engagement().debating_info().current_claim().id();
        rendering_info::FullDebateViewInfo info =
            FullDebatePageInfoParser::ParseFullDebateViewInfo(c, aliceId_, "alice", claimId);
        VRUserDatabase userDb(*db_);
        return StepView::GenerateStepViewPage(info, c, userDb, dw().getUserProtobuf(aliceId_)).DebugString();
    }

    std::string db_path_;
    Database* db_ = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
    DebateModerator* moderator_ = nullptr;
    int aliceId_ = 0;
    user::User user_;
};

TEST_F(ChatFlowTest, AQuestionIsRecordedAnsweredAndShown) {
    const int debateId = createAndEnterDebate("The bridge should close");
    ASSERT_GT(debateId, 0);

    std::string page = renderPage();
    EXPECT_NE(page.find("chatQuestionInput"), std::string::npos) << "the box is there";
    EXPECT_NE(page.find("chatQuickButton_summarize_debate"), std::string::npos);
    EXPECT_NE(page.find("chatQuickButton_summarize_claim"), std::string::npos) << "a claim is selected";
    EXPECT_EQ(page.find("aiChallengeButton"), std::string::npos) << "the AI challenger is gone";
    EXPECT_EQ(page.find("summarizeDebateButton"), std::string::npos) << "so are the summary buttons";

    auto ask = event(debate_event::ASK_CHAT);
    ask.mutable_ask_chat()->set_question("  Who holds the root claim?  ");
    send(ask);

    // Recorded at once, trimmed, and the page shows it while the model works.
    ASSERT_EQ(chat().messages_size(), 1);
    EXPECT_EQ(chat().messages(0).role(), "user");
    EXPECT_EQ(chat().messages(0).text(), "Who holds the root claim?");
    EXPECT_TRUE(chat().pending());
    EXPECT_EQ(chat().debate_id(), debateId);
    page = renderPage();
    EXPECT_NE(page.find("Who holds the root claim?"), std::string::npos);

    // The model is unreachable, so the answer is an apology -- through the
    // same path a real answer takes.
    ASSERT_TRUE(waitForAnswer(15)) << "the run never came back";
    ASSERT_EQ(chat().messages_size(), 2);
    EXPECT_EQ(chat().messages(1).role(), "assistant");
    EXPECT_NE(chat().messages(1).text().find("could not answer"), std::string::npos)
        << chat().messages(1).text();
    EXPECT_FALSE(chat().pending());

    // Both messages are in the store, in order.
    const auto stored = dw().chatTranscript(aliceId_, debateId);
    ASSERT_EQ(stored.size(), 2u);
    EXPECT_EQ(stored[0].role, "user");
    EXPECT_EQ(stored[1].role, "assistant");

    auto clear = event(debate_event::CLEAR_CHAT);
    send(clear);
    EXPECT_EQ(chat().messages_size(), 0);
    EXPECT_TRUE(dw().chatTranscript(aliceId_, debateId).empty());
}

TEST_F(ChatFlowTest, AQuickPromptAsksItsCannedQuestionAboutTheSelectedClaim) {
    createAndEnterDebate("The bridge should close");
    auto ask = event(debate_event::ASK_CHAT);
    ask.mutable_ask_chat()->set_quick_prompt(DebateChat::kSummarizeClaim);
    send(ask);
    ASSERT_EQ(chat().messages_size(), 1);
    EXPECT_NE(chat().messages(0).text().find("The bridge should close"), std::string::npos)
        << "the selected claim's sentence is in the question";
    ASSERT_TRUE(waitForAnswer(15));
}

TEST_F(ChatFlowTest, ABlankQuestionAsksNothing) {
    createAndEnterDebate("The bridge should close");
    auto ask = event(debate_event::ASK_CHAT);
    ask.mutable_ask_chat()->set_question("   ");
    send(ask);
    EXPECT_EQ(chat().messages_size(), 0);
    EXPECT_FALSE(chat().pending());
}

TEST_F(ChatFlowTest, TheTranscriptFollowsTheDebateTheUserIsIn) {
    const int first = createAndEnterDebate("First debate");
    auto ask = event(debate_event::ASK_CHAT);
    ask.mutable_ask_chat()->set_question("about the first");
    send(ask);
    ASSERT_TRUE(waitForAnswer(15));
    ASSERT_EQ(chat().messages_size(), 2);

    const int second = createAndEnterDebate("Second debate");
    ASSERT_NE(second, first);
    poke();
    EXPECT_EQ(chat().messages_size(), 0) << "a fresh conversation for the second debate";
    EXPECT_EQ(chat().debate_id(), second);

    auto back = event(debate_event::ENTER_DEBATE);
    back.mutable_enter_debate()->set_debate_id(first);
    send(back);
    poke();
    EXPECT_EQ(chat().messages_size(), 2) << "the first conversation is back from the store";
}
