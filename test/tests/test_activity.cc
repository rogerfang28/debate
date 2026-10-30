// test_activity.cc -- the admins' activity log.
//
//   ActivityTest   what is recorded (opens, the Brief, chat without its
//                  words, renames, closed accounts, visits once a day), the
//                  debate actions read from the move log, who may see it,
//                  the 90-day cleanup, and the Activity view's numbers.

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "debate.pb.h"
#include "debate_event.pb.h"
#include "moderator_to_vr.pb.h"
#include "user.pb.h"
#include "database/sqlite/Database.h"
#include "debateModerator/DebateModerator.h"
#include "debateModerator/activity/ActivityLog.h"
#include "utils/DebateWrapper.h"

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

void removeDb(const std::string& path) {
    std::remove(path.c_str());
    std::remove((path + "-wal").c_str());
    std::remove((path + "-shm").c_str());
}

constexpr int64_t kHour = 60LL * 60 * 1000;
constexpr int64_t kDay = 24 * kHour;

struct Row {
    int64_t atMs;
    int userId;
    std::string username;
    bool member;
    std::string kind;
    int debateId;
    int claimId;
    std::string detail;
};

}  // namespace

class ActivityTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = "test_activity_" + std::to_string(GETPID()) + ".sqlite3";
        removeDb(path_);
        setEnv("DB_PATH", path_);
        setEnv("DEBATE_ADMINS", "Admin");
        db_ = new Database(path_);
        moderator_ = new DebateModerator(*db_);
        admin_ = moderator_->createUserIfNotExist("admin");
        bob_ = moderator_->createUserIfNotExist("bob");
        visitor_ = moderator_->createUserIfNotExist("visitor-0000abcd");
    }
    void TearDown() override {
        delete moderator_;
        delete db_;
        setEnv("DB_PATH", "");
        setEnv("DEBATE_ADMINS", "");
        removeDb(path_);
    }

    debate_event::DebateEvent as(int userId, debate_event::EventType type) {
        debate_event::DebateEvent e;
        e.mutable_user()->set_user_id(userId);
        e.mutable_user()->set_username(moderator_->getDebateWrapper().getUserProtobuf(userId).username());
        e.mutable_user()->set_is_logged_in(true);
        e.set_type(type);
        return e;
    }
    moderator_to_vr::ModeratorToVRMessage send(debate_event::DebateEvent e) { return moderator_->handleRequest(e); }
    user::User userOf(int id) { return moderator_->getDebateWrapper().getUserProtobuf(id); }
    ActivityLog& log() { return moderator_->activity(); }

    // Every ACTIVITY row, oldest first, read from the file directly.
    std::vector<Row> rows() {
        std::vector<Row> out;
        sqlite3_stmt* stmt = db_->prepare(
            "SELECT AT_MS, USER_ID, USERNAME, IS_MEMBER, KIND, DEBATE_ID, CLAIM_ID, DETAIL FROM ACTIVITY ORDER BY ID;");
        if (!stmt) return out;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            auto str = [&](int i) {
                const unsigned char* t = sqlite3_column_text(stmt, i);
                return t ? std::string(reinterpret_cast<const char*>(t)) : std::string();
            };
            out.push_back({sqlite3_column_int64(stmt, 0), sqlite3_column_int(stmt, 1), str(2),
                           sqlite3_column_int(stmt, 3) != 0, str(4), sqlite3_column_int(stmt, 5),
                           sqlite3_column_int(stmt, 6), str(7)});
        }
        sqlite3_finalize(stmt);
        return out;
    }
    std::vector<Row> rowsOf(const std::string& kind) {
        std::vector<Row> out;
        for (const Row& r : rows()) {
            if (r.kind == kind) out.push_back(r);
        }
        return out;
    }

    // bob starts a debate; returns its id. *root is its root claim.
    int bobsDebate(int* root) {
        auto create = as(bob_, debate_event::CREATE_DEBATE);
        create.mutable_create_debate()->set_debate_topic("Cities should build more housing");
        send(create);
        const int debateId = moderator_->getDebateWrapper().getUserDebateIds(bob_).back();
        const std::vector<uint8_t> raw = moderator_->getDebateWrapper().getDebateProtobuf(debateId);
        debate::Debate d;
        d.ParseFromArray(raw.data(), static_cast<int>(raw.size()));
        *root = d.root_claim_id();
        return debateId;
    }
    void enter(int userId, int debateId) {
        auto e = as(userId, debate_event::ENTER_DEBATE);
        e.mutable_enter_debate()->set_debate_id(debateId);
        send(e);
    }
    const moderator_to_vr::ActivityEntry* find(const moderator_to_vr::ActivityReport& r, const std::string& kind) {
        for (const auto& e : r.recent()) {
            if (e.kind() == kind) return &e;
        }
        return nullptr;
    }

    std::string path_;
    Database* db_ = nullptr;
    DebateModerator* moderator_ = nullptr;
    int admin_ = 0, bob_ = 0, visitor_ = 0;
};

// ---- What is recorded --------------------------------------------------------

TEST_F(ActivityTest, OpeningADebateAndItsBriefAreRecorded) {
    int root = 0;
    const int debateId = bobsDebate(&root);
    enter(visitor_, debateId);
    send(as(visitor_, debate_event::GO_TO_BRIEF));
    auto card = as(bob_, debate_event::OPEN_DEBATE_BRIEF);
    card.mutable_enter_debate()->set_debate_id(debateId);
    send(card);

    const std::vector<Row> opens = rowsOf("open_debate");
    ASSERT_EQ(opens.size(), 2u);
    EXPECT_EQ(opens[0].userId, visitor_);
    EXPECT_EQ(opens[0].username, "visitor-0000abcd");
    EXPECT_FALSE(opens[0].member) << "a visitor";
    EXPECT_EQ(opens[0].debateId, debateId);
    EXPECT_EQ(opens[1].userId, bob_);
    EXPECT_TRUE(opens[1].member);
    EXPECT_EQ(opens[1].detail, "brief");
    ASSERT_EQ(rowsOf("brief").size(), 1u);
    EXPECT_EQ(rowsOf("brief")[0].debateId, debateId);
    EXPECT_GT(opens[0].atMs, ActivityLog::NowMs() - kHour);
}

TEST_F(ActivityTest, ADebateThatIsNotThereIsNotAnOpen) {
    enter(bob_, 999);
    EXPECT_TRUE(rowsOf("open_debate").empty());
}

TEST_F(ActivityTest, ChatIsRecordedWithoutItsWords) {
    int root = 0;
    const int debateId = bobsDebate(&root);
    enter(bob_, debateId);
    // The AI may be off in the tests, so the log is handed the event as the
    // moderator would after carrying it out.
    auto ask = as(bob_, debate_event::ASK_CHAT);
    ask.mutable_ask_chat()->set_question("my secret question about rents");
    log().recordEvent(ask, bob_);
    auto quick = as(bob_, debate_event::ASK_CHAT);
    quick.mutable_ask_chat()->set_quick_prompt("summarize_debate");
    log().recordEvent(quick, bob_);

    const std::vector<Row> chats = rowsOf("chat");
    ASSERT_EQ(chats.size(), 2u);
    EXPECT_EQ(chats[0].debateId, debateId);
    EXPECT_EQ(chats[0].detail, "");
    EXPECT_EQ(chats[1].detail, "summarize_debate");
    for (const Row& r : rows()) {
        EXPECT_EQ(r.detail.find("secret"), std::string::npos);
        EXPECT_EQ(r.username.find("secret"), std::string::npos);
    }
}

TEST_F(ActivityTest, RenameAndClosedAccounts) {
    int root = 0;
    const int debateId = bobsDebate(&root);
    enter(bob_, debateId);
    ASSERT_EQ(moderator_->renameAccount(bob_, "bobby"), "");
    const std::vector<Row> renames = rowsOf("rename");
    ASSERT_EQ(renames.size(), 1u);
    EXPECT_EQ(renames[0].username, "bobby");
    EXPECT_EQ(renames[0].detail, "bob");
    EXPECT_EQ(rowsOf("open_debate")[0].username, "bob") << "the name at the time";

    ASSERT_TRUE(moderator_->deleteAccount(bob_));
    const std::string closed = moderator_->getDebateWrapper().getUserProtobuf(bob_).username();
    ASSERT_EQ(rowsOf("delete_account").size(), 1u);
    for (const Row& r : rows()) {
        if (r.userId != bob_) continue;
        EXPECT_EQ(r.username, closed) << "a closed account keeps no old name in the log (" << r.kind << ")";
    }
}

TEST_F(ActivityTest, AVisitIsRecordedOncePerDay) {
    const int64_t now = ActivityLog::NowMs();
    log().noteSeen(visitor_, "visitor-0000abcd", /*newVisitor=*/true, now);
    log().noteSeen(visitor_, "visitor-0000abcd", false, now + 1000);
    std::vector<Row> visits = rowsOf("visit");
    ASSERT_EQ(visits.size(), 1u);
    EXPECT_EQ(visits[0].detail, "first visit");
    EXPECT_FALSE(visits[0].member);

    // Someone with anything else logged today already counts as seen.
    log().record("sign_in", bob_, "bob", 0, 0, "password", now);
    log().noteSeen(bob_, "bob", false, now + 2000);
    EXPECT_EQ(rowsOf("visit").size(), 1u);

    // The next day, once more each.
    log().noteSeen(visitor_, "visitor-0000abcd", false, now + kDay);
    log().noteSeen(bob_, "bob", false, now + kDay);
    log().noteSeen(bob_, "bob", false, now + kDay + 1000);
    visits = rowsOf("visit");
    ASSERT_EQ(visits.size(), 3u);
    EXPECT_EQ(visits[1].detail, "");
    EXPECT_TRUE(visits[2].member);
}

TEST_F(ActivityTest, DebateActionsAreReadFromTheMoveLog) {
    int root = 0;
    const int debateId = bobsDebate(&root);
    enter(admin_, debateId);
    auto agree = as(admin_, debate_event::ENDORSE_CLAIM);
    agree.mutable_endorse_claim()->set_claim_id(root);
    send(agree);
    auto withdraw = as(admin_, debate_event::UNENDORSE_CLAIM);
    withdraw.mutable_unendorse_claim()->set_claim_id(root);
    send(withdraw);

    // Nothing of them in ACTIVITY: the move log has them.
    for (const Row& r : rows()) {
        EXPECT_NE(r.kind, "agree");
        EXPECT_NE(r.kind, "new_debate");
    }
    const moderator_to_vr::ActivityReport r = log().report(ActivityLog::NowMs(), "all");
    const auto* a = find(r, "agree");
    ASSERT_NE(a, nullptr);
    EXPECT_EQ(a->user_id(), admin_);
    EXPECT_EQ(a->username(), "admin");
    EXPECT_TRUE(a->member());
    EXPECT_EQ(a->debate_id(), debateId);
    EXPECT_EQ(a->claim_id(), root);
    EXPECT_EQ(a->claim_text(), "Cities should build more housing");
    EXPECT_EQ(a->debate_title(), "Cities should build more housing");
    ASSERT_NE(find(r, "withdraw"), nullptr);
    const auto* started = find(r, "new_debate");
    ASSERT_NE(started, nullptr);
    EXPECT_EQ(started->user_id(), bob_);

    EXPECT_EQ(r.last_day().agrees(), 1);
    EXPECT_EQ(r.last_day().claims(), 1) << "the new debate's root";
    EXPECT_EQ(r.last_day().actions(), 3) << "started, agreed, withdrew";
    EXPECT_EQ(r.last_day().members(), 2) << "bob and the admin";
    EXPECT_EQ(r.days(0).actions(), 3);

    // Newest first; a visitors-only list has none of them.
    ASSERT_GE(r.recent_size(), 2);
    EXPECT_EQ(r.recent(0).kind(), "withdraw");
    const moderator_to_vr::ActivityReport visitors = log().report(ActivityLog::NowMs(), "visitors");
    for (const auto& e : visitors.recent()) EXPECT_FALSE(e.member()) << e.kind();
    EXPECT_EQ(visitors.filter(), "visitors");
}

TEST_F(ActivityTest, AnImportIsOneEntryNotEveryClaim) {
    send(as(admin_, debate_event::IMPORT_OPEN));
    const std::string outline = "Root claim | Dana\n  + A support | Evan\n  - A challenge | Fay\n";
    auto preview = as(admin_, debate_event::IMPORT_PREVIEW);
    preview.mutable_import_debate()->set_text(outline);
    send(preview);
    auto publish = as(admin_, debate_event::IMPORT_PUBLISH);
    publish.mutable_import_debate()->set_text(outline);
    send(publish);

    const std::vector<Row> imports = rowsOf("import");
    ASSERT_EQ(imports.size(), 1u);
    EXPECT_EQ(imports[0].detail, "3 claims");
    EXPECT_GT(imports[0].debateId, 0);
    const moderator_to_vr::ActivityReport r = log().report(ActivityLog::NowMs(), "all");
    EXPECT_EQ(r.last_day().actions(), 0) << "the import's own claims are not counted as actions";
    ASSERT_NE(find(r, "import"), nullptr);
    EXPECT_EQ(find(r, "support"), nullptr);
}

// ---- Who may see it ----------------------------------------------------------

TEST_F(ActivityTest, OnlyAdminsGetTheActivityView) {
    auto bobAsks = send(as(bob_, debate_event::ACTIVITY_OPEN));
    EXPECT_FALSE(bobAsks.has_activity());
    EXPECT_FALSE(userOf(bob_).engagement().home_view().activity_open());
    auto visitorAsks = send(as(visitor_, debate_event::ACTIVITY_OPEN));
    EXPECT_FALSE(visitorAsks.has_activity());

    // A record that says it is open (a stale page, a hand-made change) still
    // gets nothing when its owner is not an admin.
    user::User bob = userOf(bob_);
    bob.mutable_engagement()->mutable_home_view()->set_activity_open(true);
    bob.mutable_engagement()->mutable_home_view()->set_profile_user_id(bob_);
    moderator_->getDebateWrapper().updateUserProtobuf(bob_, bob);
    EXPECT_FALSE(send(as(bob_, debate_event::NONE)).has_activity());

    auto adminAsks = send(as(admin_, debate_event::ACTIVITY_OPEN));
    ASSERT_TRUE(adminAsks.has_activity());
    EXPECT_EQ(adminAsks.activity().filter(), "all");
    EXPECT_EQ(adminAsks.activity().days_size(), ActivityLog::kDays);
    EXPECT_EQ(userOf(admin_).engagement().home_view().profile_user_id(), admin_) << "on their own profile";

    auto filter = as(admin_, debate_event::ACTIVITY_FILTER);
    filter.mutable_explore_option()->set_value("members");
    EXPECT_EQ(send(filter).activity().filter(), "members");

    auto close = send(as(admin_, debate_event::ACTIVITY_CLOSE));
    EXPECT_FALSE(close.has_activity());
    EXPECT_FALSE(userOf(admin_).engagement().home_view().activity_open());
    EXPECT_EQ(userOf(admin_).engagement().home_view().profile_user_id(), admin_) << "back on the profile";

    // Going home closes it too.
    send(as(admin_, debate_event::ACTIVITY_OPEN));
    EXPECT_FALSE(send(as(admin_, debate_event::GO_HOME)).has_activity());
}

// ---- Keeping it 90 days ------------------------------------------------------

TEST_F(ActivityTest, RowsOlderThan90DaysAreDeleted) {
    const int64_t now = ActivityLog::NowMs();
    log().record("visit", visitor_, "visitor-0000abcd", 0, 0, "", now - 91 * kDay);
    log().record("sign_in", bob_, "bob", 0, 0, "password", now - 90 * kDay - kHour);
    log().record("sign_in", bob_, "bob", 0, 0, "password", now - 89 * kDay);
    log().record("visit", admin_, "admin", 0, 0, "", now - kHour);
    EXPECT_EQ(log().prune(now), 2);
    const std::vector<Row> left = rows();
    ASSERT_EQ(left.size(), 2u);
    EXPECT_EQ(left[0].atMs, now - 89 * kDay);
    EXPECT_EQ(log().prune(now), 0);
}

// ---- The numbers -------------------------------------------------------------

TEST_F(ActivityTest, SummaryDaysAndMostOpened) {
    const int64_t now = ActivityLog::NowMs();
    // The last 24 hours: two visitors (one of them twice), bob signs in and
    // opens debate 7 twice, the admin opens it once and debate 8 once.
    log().record("visit", 900, "visitor-00000001", 0, 0, "first visit", now - 2 * kHour);
    log().record("visit", 901, "visitor-00000002", 0, 0, "first visit", now - 3 * kHour);
    log().record("open_debate", 901, "visitor-00000002", 8, 0, "", now - 3 * kHour + 1000);
    log().record("sign_in", bob_, "bob", 0, 0, "password", now - kHour);
    log().record("open_debate", bob_, "bob", 7, 0, "", now - kHour + 1000);
    log().record("open_debate", bob_, "bob", 7, 0, "", now - kHour + 2000);
    log().record("open_debate", admin_, "admin", 7, 0, "", now - 30 * 60 * 1000);
    // Earlier in the week: a third visitor, and carol's new account.
    log().record("visit", 902, "visitor-00000003", 0, 0, "first visit", now - 3 * kDay);
    log().record("sign_up", 903, "carol", 0, 0, "Google", now - 2 * kDay);
    // Older than the week, inside the 14 days.
    log().record("sign_in", bob_, "bob", 0, 0, "password", now - 10 * kDay);
    // Older than 14 days: in no number.
    log().record("visit", 904, "visitor-00000004", 0, 0, "", now - 20 * kDay);

    const moderator_to_vr::ActivityReport r = log().report(now, "all");
    EXPECT_EQ(r.last_day().visitors(), 2);
    EXPECT_EQ(r.last_day().members(), 2) << "bob and the admin";
    EXPECT_EQ(r.last_day().sign_ins(), 1);
    EXPECT_EQ(r.last_day().new_accounts(), 0);
    EXPECT_EQ(r.last_day().debates_opened(), 4);
    EXPECT_EQ(r.last_day().actions(), 0);

    EXPECT_EQ(r.last_week().visitors(), 3);
    EXPECT_EQ(r.last_week().members(), 3) << "bob, the admin, carol";
    EXPECT_EQ(r.last_week().sign_ins(), 1);
    EXPECT_EQ(r.last_week().new_accounts(), 1);

    ASSERT_EQ(r.days_size(), 14);
    EXPECT_EQ(r.days(0).date(), ActivityDatabase::IsoFromMs(now).substr(0, 10));
    int visitors = 0, signIns = 0, newAccounts = 0;
    for (const auto& d : r.days()) {
        visitors += d.visitors();
        signIns += d.sign_ins();
        newAccounts += d.new_accounts();
    }
    EXPECT_EQ(visitors, 3) << "the 20-day-old visit is outside the 14 days";
    EXPECT_EQ(signIns, 2);
    EXPECT_EQ(newAccounts, 1);
    EXPECT_EQ(r.days(10).sign_ins(), 1);
    EXPECT_EQ(r.days(3).visitors(), 1);
    EXPECT_EQ(r.days(2).new_accounts(), 1);

    ASSERT_EQ(r.top_debates_size(), 2);
    EXPECT_EQ(r.top_debates(0).debate_id(), 7);
    EXPECT_EQ(r.top_debates(0).opens(), 3);
    EXPECT_EQ(r.top_debates(0).people(), 2);
    EXPECT_EQ(r.top_debates(1).debate_id(), 8);

    // Newest first; the filter picks members or visitors.
    ASSERT_GE(r.recent_size(), 3);
    EXPECT_EQ(r.recent(0).kind(), "open_debate");
    EXPECT_EQ(r.recent(0).user_id(), admin_);
    const moderator_to_vr::ActivityReport members = log().report(now, "members");
    ASSERT_GT(members.recent_size(), 0);
    for (const auto& e : members.recent()) EXPECT_TRUE(e.member()) << e.username();
    const moderator_to_vr::ActivityReport visitorsOnly = log().report(now, "visitors");
    EXPECT_EQ(visitorsOnly.recent_size(), 5);
    for (const auto& e : visitorsOnly.recent()) EXPECT_FALSE(e.member()) << e.username();
}
