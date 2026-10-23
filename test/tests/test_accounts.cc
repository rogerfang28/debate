// test_accounts.cc -- usernames, profiles and the account jobs.
//
//   UsernamePolicyTest   the rules a new name must follow, names made from
//                        an email, the "Deleted account" scrub
//   AccountServiceTest   rename, delete, Google lookup, the email-name
//                        migration (both user tables) and the visitor sweep
//   ProfileTest          the profile's statistics, built through the
//                        moderator from real events, across a rename and a
//                        deletion

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

#include "debate.pb.h"
#include "debate_event.pb.h"
#include "user.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "debateModerator/DebateModerator.h"
#include "debateModerator/accounts/AccountService.h"
#include "utils/DebateWrapper.h"
#include "utils/UsernamePolicy.h"

#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

namespace up = username_policy;

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

std::vector<uint8_t> bytesOf(const user::User& u) {
    const std::string s = u.SerializeAsString();
    return std::vector<uint8_t>(s.begin(), s.end());
}

user::User parse(const std::vector<uint8_t>& blob) {
    user::User u;
    if (!blob.empty()) u.ParseFromArray(blob.data(), static_cast<int>(blob.size()));
    return u;
}

}  // namespace

// ===========================================================================
// UsernamePolicy
// ===========================================================================

TEST(UsernamePolicyTest, AcceptsNamesThatFollowTheRules) {
    for (const std::string name : {"bob", "Bob_Smith", "a.b-c_d", "123", "x9y", "abcdefghijklmnopqrstuvwx"}) {
        EXPECT_EQ(up::ShapeError(name), "") << name;
    }
}

TEST(UsernamePolicyTest, RefusesLengthCharactersAndFirstCharacter) {
    EXPECT_NE(up::ShapeError(""), "");
    EXPECT_NE(up::ShapeError("ab"), "");
    EXPECT_NE(up::ShapeError("abcdefghijklmnopqrstuvwxy"), "");  // 25
    EXPECT_NE(up::ShapeError("has space"), "");
    EXPECT_NE(up::ShapeError("semi;colon"), "");
    EXPECT_NE(up::ShapeError("caf\xC3\xA9"), "");  // non-ASCII
    EXPECT_NE(up::ShapeError("_bob"), "");
    EXPECT_NE(up::ShapeError(".bob"), "");
    EXPECT_NE(up::ShapeError("-bob"), "");
}

TEST(UsernamePolicyTest, ReservesVisitorDeletedAndEmails) {
    EXPECT_NE(up::ShapeError("visitor-1234abcd"), "");
    EXPECT_NE(up::ShapeError("Visitor-me"), "");
    EXPECT_NE(up::ShapeError("deleted-12"), "");
    EXPECT_NE(up::ShapeError("DELETED-x"), "");
    EXPECT_NE(up::ShapeError("bob@example.com"), "");
    EXPECT_NE(up::ShapeError("a@b"), "");
    // Close but not reserved.
    EXPECT_EQ(up::ShapeError("visitors"), "");
    EXPECT_EQ(up::ShapeError("deletedfan"), "");
}

TEST(UsernamePolicyTest, CheckAddsUniqueness) {
    auto takenBob = [](const std::string& n) { return up::Lower(n) == "bob"; };
    EXPECT_NE(up::Check("bob", takenBob), "");
    EXPECT_NE(up::Check("BOB", takenBob), "");
    EXPECT_EQ(up::Check("bobby", takenBob), "");
    // A shape problem is reported before uniqueness is asked.
    EXPECT_NE(up::Check("b", [](const std::string&) { return false; }), "");
}

TEST(UsernamePolicyTest, BaseFromEmailFollowsTheRules) {
    EXPECT_EQ(up::BaseFromEmail("bob@gmail.com"), "bob");
    EXPECT_EQ(up::BaseFromEmail("Bob.Smith+news@x.org"), "Bob.Smith_news");
    EXPECT_EQ(up::BaseFromEmail("a@x.com"), "a00");
    EXPECT_EQ(up::BaseFromEmail("_.x@x.com"), "x00");
    EXPECT_EQ(up::BaseFromEmail("visitor-12345678@x.com"), "uvisitor-12345678");
    EXPECT_EQ(up::BaseFromEmail("deleted-3@x.com"), "udeleted-3");
    EXPECT_EQ(up::BaseFromEmail("\xE6\x97\xA5\xE6\x9C\xAC@x.jp"), "user");
    const std::string longName = up::BaseFromEmail("averyveryveryverylongname.indeed@x.com");
    EXPECT_EQ(longName.size(), up::kMaxLength);
    for (const std::string email : {"bob@gmail.com", "Bob.Smith+news@x.org", "a@x.com", "_.x@x.com",
                                    "visitor-12345678@x.com", "averyveryveryverylongname.indeed@x.com",
                                    "x-------------------------y@z.com"}) {
        EXPECT_EQ(up::ShapeError(up::BaseFromEmail(email)), "") << email;
    }
}

TEST(UsernamePolicyTest, MakeUniqueCountsUpAndStaysInLength) {
    std::vector<std::string> taken = {"bob", "bob2"};
    auto isTaken = [&](const std::string& n) {
        for (const auto& t : taken) if (up::Lower(t) == up::Lower(n)) return true;
        return false;
    };
    EXPECT_EQ(up::MakeUnique("alice", isTaken), "alice");
    EXPECT_EQ(up::MakeUnique("bob", isTaken), "bob3");
    EXPECT_EQ(up::MakeUnique("BOB", isTaken), "BOB3");
    const std::string full(24, 'z');
    taken = {full};
    const std::string next = up::MakeUnique(full, isTaken);
    EXPECT_EQ(next.size(), up::kMaxLength);
    EXPECT_EQ(next, std::string(23, 'z') + "2");
}

TEST(UsernamePolicyTest, DeletedNamesBecomeDeletedAccount) {
    EXPECT_EQ(up::DeletedName(17), "deleted-17");
    EXPECT_TRUE(up::IsDeletedName("deleted-17"));
    EXPECT_FALSE(up::IsDeletedName("deleted-"));
    EXPECT_FALSE(up::IsDeletedName("deleted-1a"));
    EXPECT_EQ(up::DisplayName("deleted-17"), "Deleted account");
    EXPECT_EQ(up::DisplayName("bob"), "bob");

    EXPECT_EQ(up::ScrubDeletedNames("deleted-17"), "Deleted account");
    EXPECT_EQ(up::ScrubDeletedNames("deleted-17's move"), "Deleted account's move");
    EXPECT_EQ(up::ScrubDeletedNames("bob, deleted-3, deleted-40"), "bob, Deleted account, Deleted account");
    EXPECT_EQ(up::ScrubDeletedNames("Claimed by deleted-5."), "Claimed by Deleted account.");
    // Not a closed account's name.
    EXPECT_EQ(up::ScrubDeletedNames("undeleted-3"), "undeleted-3");
    EXPECT_EQ(up::ScrubDeletedNames("deleted-3a"), "deleted-3a");
    EXPECT_EQ(up::ScrubDeletedNames("deleted-x"), "deleted-x");
    EXPECT_EQ(up::ScrubDeletedNames("nothing here"), "nothing here");
}

// ===========================================================================
// AccountService
// ===========================================================================

class AccountServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = "test_accounts_" + std::to_string(GETPID()) + ".sqlite3";
        legacyPath_ = "test_accounts_legacy_" + std::to_string(GETPID()) + ".sqlite3";
        removeDb(path_);
        removeDb(legacyPath_);
        db_ = new Database(path_);
        legacyDb_ = new Database(legacyPath_);
        wrapper_ = new DatabaseWrapper(*db_, *db_);
        ASSERT_TRUE(wrapper_->ensureAllTables());
        // The debates file's old USERS table, as an old server has it.
        ASSERT_TRUE(legacyDb_->execute(
            "CREATE TABLE USERS (ID INTEGER PRIMARY KEY AUTOINCREMENT, USERNAME TEXT NOT NULL UNIQUE, USER_DATA BLOB);"));
        // Debate tables live in the users file in this fixture; the service
        // reads them from its debates database, which here is that same file.
        accounts_ = new AccountService(*wrapper_, *db_, *legacyDb_);
        sameFile_ = new AccountService(*wrapper_, *db_, *db_);
    }
    void TearDown() override {
        delete sameFile_;
        delete accounts_;
        delete wrapper_;
        delete legacyDb_;
        delete db_;
        removeDb(path_);
        removeDb(legacyPath_);
    }

    int addUser(const std::string& name, const std::string& sub = "", const std::string& email = "",
                const std::string& passwordHash = "", int64_t joinedAt = 0) {
        user::User u;
        u.set_username(name);
        u.set_google_sub(sub);
        u.set_email(email);
        u.set_password_hash(passwordHash);
        u.set_joined_at(joinedAt);
        const int id = wrapper_->users.createUser(name, bytesOf(u));
        u.set_user_id(id);
        wrapper_->users.updateUserProtobuf(id, bytesOf(u));
        return id;
    }

    void addLegacyRow(const std::string& name) {
        user::User u;
        u.set_username(name);
        u.set_email(name);
        const std::string blob = u.SerializeAsString();
        sqlite3_stmt* stmt = legacyDb_->prepare("INSERT INTO USERS (USERNAME, USER_DATA) VALUES (?, ?);");
        ASSERT_NE(stmt, nullptr);
        sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_blob(stmt, 2, blob.data(), static_cast<int>(blob.size()), SQLITE_TRANSIENT);
        ASSERT_EQ(sqlite3_step(stmt), SQLITE_DONE);
        sqlite3_finalize(stmt);
    }

    std::vector<std::string> legacyNames() {
        std::vector<std::string> out;
        sqlite3_stmt* stmt = legacyDb_->prepare("SELECT USERNAME FROM USERS ORDER BY ID;");
        while (stmt && sqlite3_step(stmt) == SQLITE_ROW) {
            out.emplace_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
        }
        if (stmt) sqlite3_finalize(stmt);
        return out;
    }

    user::User userOf(int id) { return parse(wrapper_->users.getUserProtobuf(id)); }

    std::string path_, legacyPath_;
    Database* db_ = nullptr;
    Database* legacyDb_ = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
    AccountService* accounts_ = nullptr;
    AccountService* sameFile_ = nullptr;
};

TEST_F(AccountServiceTest, MigrationRenamesGoogleEmailNamesInBothTables) {
    const int bob = addUser("bob@gmail.com", "sub-bob", "bob@gmail.com");
    addUser("bob");  // an existing "bob": the Google account becomes bob2
    const int old = addUser("Old.Timer@example.org");  // the old Google sign-in: no link, no password
    const int pw = addUser("pw@example.com", "", "", "pbkdf2$1$00$00");  // chose an email with a password
    const int plain = addUser("carol", "sub-carol", "carol@x.com");
    addLegacyRow("bob@gmail.com");
    addLegacyRow("Old.Timer@example.org");
    addLegacyRow("stranger@x.com");  // only the old table has it

    EXPECT_EQ(accounts_->MigrateEmailUsernames(), 2);

    EXPECT_EQ(wrapper_->users.getUsername(bob), "bob2");
    EXPECT_EQ(userOf(bob).username(), "bob2");
    EXPECT_EQ(userOf(bob).email(), "bob@gmail.com") << "the email stays, privately";
    EXPECT_EQ(userOf(bob).google_sub(), "sub-bob");

    EXPECT_EQ(wrapper_->users.getUsername(old), "Old.Timer");
    EXPECT_EQ(userOf(old).email(), "Old.Timer@example.org") << "so the next Google sign-in can find it";

    EXPECT_EQ(wrapper_->users.getUsername(pw), "pw@example.com") << "a password account's own choice";
    EXPECT_EQ(wrapper_->users.getUsername(plain), "carol");

    const std::vector<std::string> legacy = legacyNames();
    ASSERT_EQ(legacy.size(), 3u);
    EXPECT_EQ(legacy[0], "bob2");
    EXPECT_EQ(legacy[1], "Old.Timer");
    EXPECT_EQ(legacy[2], "stranger");
    for (const std::string& name : legacy) EXPECT_EQ(name.find('@'), std::string::npos);

    // Nothing left to do on the next start.
    EXPECT_EQ(accounts_->MigrateEmailUsernames(), 0);
    EXPECT_EQ(legacyNames(), legacy);
}

TEST_F(AccountServiceTest, MigrationWithOneUsersTableLeavesTheDebateTablesAlone) {
    const int bob = addUser("bob@gmail.com", "sub-bob", "bob@gmail.com");
    EXPECT_EQ(sameFile_->MigrateEmailUsernames(), 1);
    EXPECT_EQ(wrapper_->users.getUsername(bob), "bob");
}

TEST_F(AccountServiceTest, GoogleLookupIsBySubThenByUnclaimedEmail) {
    const int bob = addUser("bob", "sub-bob", "bob@gmail.com");
    const int old = addUser("old@x.com");
    const int pw = addUser("pat", "", "pat@x.com", "pbkdf2$1$00$00");
    EXPECT_EQ(accounts_->FindByGoogleSub("sub-bob"), bob);
    EXPECT_EQ(accounts_->FindByGoogleSub("sub-nobody"), -1);
    EXPECT_EQ(accounts_->FindUnclaimedByEmail("OLD@x.com"), old);
    EXPECT_EQ(accounts_->FindUnclaimedByEmail("bob@gmail.com"), -1) << "already linked to a Google account";
    EXPECT_EQ(accounts_->FindUnclaimedByEmail("pat@x.com"), -1) << "a password account is not Google's";
    (void)pw;
    EXPECT_EQ(accounts_->UniqueNameFromEmail("bob@other.com"), "bob2");
    EXPECT_EQ(accounts_->UniqueNameFromEmail("BOB@other.com"), "BOB2");
    EXPECT_EQ(accounts_->UniqueNameFromEmail("new.person@x.com"), "new.person");
}

TEST_F(AccountServiceTest, RenameFollowsTheRules) {
    const int bob = addUser("bob");
    addUser("alice");
    const int visitor = addUser("visitor-0123abcd");

    EXPECT_NE(accounts_->Rename(bob, "ALICE"), "") << "taken, ignoring case";
    EXPECT_NE(accounts_->Rename(bob, "x"), "");
    EXPECT_NE(accounts_->Rename(bob, "me@x.com"), "");
    EXPECT_NE(accounts_->Rename(bob, "deleted-9"), "");
    EXPECT_EQ(wrapper_->users.getUsername(bob), "bob");

    EXPECT_EQ(accounts_->Rename(bob, "Bob"), "") << "your own name in another case is yours";
    EXPECT_EQ(accounts_->Rename(bob, "robert"), "");
    EXPECT_EQ(wrapper_->users.getUsername(bob), "robert");
    EXPECT_EQ(userOf(bob).username(), "robert");
    EXPECT_EQ(accounts_->Rename(bob, "robert"), "") << "no change is not an error";

    EXPECT_NE(accounts_->Rename(visitor, "visitorname"), "") << "a visitor has no account to rename";
}

TEST_F(AccountServiceTest, DeleteWipesTheAccountButKeepsMembership) {
    const int bob = addUser("bob", "sub-bob", "bob@gmail.com", "pbkdf2$1$00$00", 1700000000);
    user::User u = userOf(bob);
    u.mutable_engagement()->mutable_home_view()->set_explore_query("secret search");
    wrapper_->users.updateUserProtobuf(bob, bytesOf(u));
    wrapper_->chats.append(bob, 5, "user", "my question");
    wrapper_->chats.append(bob, 6, "assistant", "an answer");
    wrapper_->debateMembers.addMember(5, bob);
    addLegacyRow("bob");

    ASSERT_TRUE(accounts_->Delete(bob));

    EXPECT_EQ(wrapper_->users.getUsername(bob), "deleted-" + std::to_string(bob));
    const user::User closed = userOf(bob);
    EXPECT_EQ(closed.username(), "deleted-" + std::to_string(bob));
    EXPECT_EQ(closed.email(), "");
    EXPECT_EQ(closed.google_sub(), "");
    EXPECT_EQ(closed.password_hash(), "");
    EXPECT_EQ(closed.joined_at(), 0);
    EXPECT_EQ(closed.engagement().home_view().explore_query(), "");
    EXPECT_TRUE(wrapper_->chats.list(bob, 5).empty());
    EXPECT_TRUE(wrapper_->chats.list(bob, 6).empty());
    EXPECT_TRUE(wrapper_->debateMembers.isMember(5, bob)) << "their claims are built from membership";
    EXPECT_EQ(legacyNames(), std::vector<std::string>{"deleted-" + std::to_string(bob)});

    EXPECT_EQ(accounts_->FindByGoogleSub("sub-bob"), -1) << "Google can never find it again";
    EXPECT_FALSE(accounts_->Delete(bob)) << "already closed";
    EXPECT_NE(accounts_->Rename(bob, "phoenix"), "") << "a closed account stays closed";
}

TEST_F(AccountServiceTest, SweepRemovesOnlyOldUnusedVisitors) {
    const int64_t now = 2000000000;
    const int64_t day = 24 * 60 * 60;
    const int old = addUser("visitor-0000000a", "", "", "", now - 4 * day);
    const int noDate = addUser("visitor-0000000b");
    const int young = addUser("visitor-0000000c", "", "", "", now - 2 * day);
    const int withPassword = addUser("visitor-0000000d", "", "", "pbkdf2$1$00$00");
    const int oddShape = addUser("visitor-XYZ");
    const int entered = addUser("visitor-0000000e");
    const int person = addUser("alice");
    wrapper_->chats.append(old, 3, "user", "hello");
    wrapper_->debateMembers.addMember(3, old);
    // A claim entered by `entered`: the sweep keeps it (a debate needs it).
    debate::Claim claim;
    claim.set_sentence("Visitors cannot do this, but say one did");
    const std::string claimBytes = claim.SerializeAsString();
    wrapper_->statements.addStatement(0, claim.sentence(),
                                      std::vector<uint8_t>(claimBytes.begin(), claimBytes.end()), entered, 3);

    EXPECT_EQ(sameFile_->SweepVisitors(now), 2);

    EXPECT_EQ(wrapper_->users.getUsername(old), "");
    EXPECT_EQ(wrapper_->users.getUsername(noDate), "");
    EXPECT_TRUE(wrapper_->chats.list(old, 3).empty());
    EXPECT_FALSE(wrapper_->debateMembers.isMember(3, old));
    EXPECT_EQ(wrapper_->users.getUsername(young), "visitor-0000000c");
    EXPECT_EQ(wrapper_->users.getUsername(withPassword), "visitor-0000000d");
    EXPECT_EQ(wrapper_->users.getUsername(oddShape), "visitor-XYZ");
    EXPECT_EQ(wrapper_->users.getUsername(entered), "visitor-0000000e");
    EXPECT_EQ(wrapper_->users.getUsername(person), "alice");

    EXPECT_EQ(sameFile_->SweepVisitors(now), 0);
}

// ===========================================================================
// Profiles, through the moderator
// ===========================================================================

class ProfileTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = "test_profile_" + std::to_string(GETPID()) + ".sqlite3";
        removeDb(path_);
        setEnv("DB_PATH", path_);
        db_ = new Database(path_);
        moderator_ = new DebateModerator(*db_);
        alice_ = moderator_->createUserIfNotExist("alice");
        bob_ = moderator_->createUserIfNotExist("bob");
    }
    void TearDown() override {
        delete moderator_;
        delete db_;
        setEnv("DB_PATH", "");
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
    moderator_to_vr::ModeratorToVRMessage send(debate_event::DebateEvent e) {
        return moderator_->handleRequest(e);
    }
    int currentClaim(int userId) {
        return moderator_->getDebateWrapper().getUserProtobuf(userId).engagement().debating_info().current_claim().id();
    }
    moderator_to_vr::Profile profileOf(int viewer, int target) {
        auto open = as(viewer, debate_event::OPEN_PROFILE);
        open.mutable_open_profile()->set_user_id(target);
        return send(open).profile();
    }

    // alice: a debate with a root and one supporting claim. bob: challenges
    // the root, agrees with the supporting claim.
    int buildDebate() {
        auto create = as(alice_, debate_event::CREATE_DEBATE);
        create.mutable_create_debate()->set_debate_topic("Cities should build more housing");
        send(create);
        const int debateId = moderator_->getDebateWrapper().getUserDebateIds(alice_).back();
        auto enter = as(alice_, debate_event::ENTER_DEBATE);
        enter.mutable_enter_debate()->set_debate_id(debateId);
        send(enter);
        const int root = currentClaim(alice_);
        auto add = as(alice_, debate_event::ADD_CHILD_CLAIM);
        add.mutable_add_child_claim()->set_claim("Supply lowers rents");
        send(add);
        int support = 0;
        for (const auto& blob : moderator_->getDebateWrapper().getStatementsForDebate(debateId)) {
            debate::Claim c;
            if (c.ParseFromArray(blob.data(), static_cast<int>(blob.size())) && c.sentence() == "Supply lowers rents") {
                support = c.id();
            }
        }

        auto benter = as(bob_, debate_event::ENTER_DEBATE);
        benter.mutable_enter_debate()->set_debate_id(debateId);
        send(benter);
        send(as(bob_, debate_event::START_CHALLENGE_CLAIM));
        auto target = as(bob_, debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
        target.mutable_add_claim_to_be_challenged()->set_claim_id(root);
        send(target);
        send(as(bob_, debate_event::OPEN_ADD_CHALLENGE));
        auto submit = as(bob_, debate_event::SUBMIT_CHALLENGE_CLAIM);
        submit.mutable_submit_challenge_claim()->set_challenge_sentence("Zoning is not the bottleneck");
        send(submit);
        if (support > 0) {
            auto agree = as(bob_, debate_event::ENDORSE_CLAIM);
            agree.mutable_endorse_claim()->set_claim_id(support);
            send(agree);
        }
        return debateId;
    }

    std::string path_;
    Database* db_ = nullptr;
    DebateModerator* moderator_ = nullptr;
    int alice_ = 0;
    int bob_ = 0;
};

TEST_F(ProfileTest, NewAccountsHaveAJoinDate) {
    EXPECT_GT(moderator_->getDebateWrapper().getUserProtobuf(alice_).joined_at(), 1700000000);
}

TEST_F(ProfileTest, StatisticsComeFromMovesAndClaims) {
    const int debateId = buildDebate();

    const moderator_to_vr::Profile alice = profileOf(bob_, alice_);
    EXPECT_EQ(alice.kind(), "member");
    EXPECT_EQ(alice.username(), "alice");
    EXPECT_FALSE(alice.own());
    EXPECT_EQ(alice.debates(), 1);
    ASSERT_EQ(alice.debate_ids_size(), 1);
    EXPECT_EQ(alice.debate_ids(0), debateId);
    EXPECT_EQ(alice.claims(), 2);
    EXPECT_EQ(alice.challenges(), 0);
    EXPECT_EQ(alice.open_challenges(), 1) << "bob's challenge waits on alice";

    const moderator_to_vr::Profile bob = profileOf(bob_, bob_);
    EXPECT_TRUE(bob.own());
    EXPECT_EQ(bob.debates(), 1);
    EXPECT_EQ(bob.claims(), 0);
    EXPECT_EQ(bob.challenges(), 1);
    EXPECT_EQ(bob.agreements(), 1);
    EXPECT_EQ(bob.open_challenges(), 0);
}

TEST_F(ProfileTest, AVoiceIsCreditedWithWhatWasEnteredInTheirName) {
    buildDebate();
    // alice enters a claim in Paul's voice under her supporting claim.
    auto add = as(alice_, debate_event::ADD_CHILD_CLAIM);
    add.mutable_add_child_claim()->set_claim("Paul said rents fell after the rezoning");
    add.mutable_add_child_claim()->set_voice("Paul");
    send(add);
    const int paul = moderator_->getUserId("Paul");
    ASSERT_GT(paul, 0);
    const moderator_to_vr::Profile p = profileOf(alice_, paul);
    EXPECT_EQ(p.kind(), "voice");
    EXPECT_EQ(p.claims(), 1);
    EXPECT_EQ(p.debates(), 1);
}

TEST_F(ProfileTest, RenameKeepsEverythingAndRefusesInPlace) {
    buildDebate();
    profileOf(bob_, bob_);

    EXPECT_NE(moderator_->renameAccount(bob_, "Alice"), "");
    user::User refused = moderator_->getDebateWrapper().getUserProtobuf(bob_);
    EXPECT_EQ(refused.username(), "bob");
    EXPECT_EQ(refused.engagement().home_view().profile_panel(), "rename");
    EXPECT_NE(refused.engagement().home_view().profile_error(), "");
    EXPECT_EQ(refused.engagement().home_view().profile_draft(), "Alice");

    EXPECT_EQ(moderator_->renameAccount(bob_, "  robert "), "");
    const user::User renamed = moderator_->getDebateWrapper().getUserProtobuf(bob_);
    EXPECT_EQ(renamed.username(), "robert");
    EXPECT_EQ(renamed.engagement().home_view().profile_panel(), "");
    EXPECT_EQ(renamed.engagement().home_view().profile_user_id(), bob_);

    const moderator_to_vr::Profile bob = profileOf(bob_, bob_);
    EXPECT_EQ(bob.username(), "robert");
    EXPECT_EQ(bob.challenges(), 1);
    EXPECT_EQ(bob.agreements(), 1);
    EXPECT_EQ(profileOf(bob_, alice_).open_challenges(), 1) << "still waiting on alice, from robert";
}

TEST_F(ProfileTest, ADeletedAccountHasNoProfileButItsClaimsStay) {
    const int debateId = buildDebate();
    const size_t claimsBefore = moderator_->getDebateWrapper().getStatementsForDebate(debateId).size();
    ASSERT_TRUE(moderator_->deleteAccount(bob_));
    const moderator_to_vr::Profile bob = profileOf(alice_, bob_);
    EXPECT_EQ(bob.kind(), "deleted");
    EXPECT_EQ(bob.username(), "") << "nothing about a closed account is shown";
    EXPECT_EQ(bob.debates(), 0);
    EXPECT_EQ(moderator_->getDebateWrapper().getStatementsForDebate(debateId).size(), claimsBefore);
    EXPECT_EQ(profileOf(alice_, alice_).open_challenges(), 1) << "the challenge still stands";
}

TEST_F(ProfileTest, ChoosingATabClosesTheProfile) {
    profileOf(alice_, bob_);
    EXPECT_EQ(moderator_->getDebateWrapper().getUserProtobuf(alice_).engagement().home_view().profile_user_id(), bob_);
    auto tab = as(alice_, debate_event::GO_TO_HOME_TAB);
    tab.mutable_home_tab()->set_tab("explore");
    send(tab);
    EXPECT_EQ(moderator_->getDebateWrapper().getUserProtobuf(alice_).engagement().home_view().profile_user_id(), 0);
}

TEST_F(ProfileTest, ProfilePanelIsYourOwnProfileOnly) {
    profileOf(alice_, bob_);
    auto panel = as(alice_, debate_event::PROFILE_PANEL);
    panel.mutable_explore_option()->set_value("delete");
    send(panel);
    EXPECT_EQ(moderator_->getDebateWrapper().getUserProtobuf(alice_).engagement().home_view().profile_panel(), "");
    profileOf(alice_, alice_);
    send(panel);
    EXPECT_EQ(moderator_->getDebateWrapper().getUserProtobuf(alice_).engagement().home_view().profile_panel(), "delete");
}

TEST_F(ProfileTest, SameAccountMadeByConcurrentRequestsIsOneAccount) {
    // A visitor's open tabs all arrive at once after the sweep removed the
    // name their browser still sends: every request must get the one account.
    std::vector<int> ids(8, 0);
    std::vector<std::thread> threads;
    for (size_t i = 0; i < ids.size(); ++i) {
        threads.emplace_back([&, i]() { ids[i] = moderator_->createUserIfNotExist("visitor-0badcafe"); });
    }
    for (auto& t : threads) t.join();
    const int expected = moderator_->getUserId("visitor-0badcafe");
    ASSERT_GT(expected, 0);
    for (const int id : ids) EXPECT_EQ(id, expected);
}
