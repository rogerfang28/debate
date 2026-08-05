// test_move_log.cc — Append-only move log
//
// Two layers are covered here, for different reasons.
//
// MoveDatabaseTest  — the storage primitive on its own: sequence assignment,
//                     debate scoping, and the guarantee that a failing log
//                     write cannot break the operation that triggered it.
//
// MoveLogTest       — the real pipeline, driven through DebateModerator with
//                     forged DebateEvents. This matters because MoveLogger
//                     hangs off event dispatch, so calling handlers directly
//                     (as the other test files do) would bypass it entirely
//                     and prove nothing.
//
// These assert EXACT move counts rather than "at least one". A move logged
// twice is as wrong as a move not logged at all, and the second kind of bug
// is invisible unless the count is pinned down. The regression that prompted
// this file -- CREATE_DEBATE silently logging nothing, because the handler
// does not move the user into the debate it creates -- passed a clean build
// and 15 green tests. Only counting rows caught it.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <set>
#include <thread>
#include <atomic>
#include <cstdio>
#include <cstdlib>

#include "debate.pb.h"
#include "debate_event.pb.h"
#include "user.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "utils/DebateWrapper.h"
#include "debateModerator/DebateModerator.h"

namespace {

struct MoveRow {
    int seq = 0;
    int actorId = 0;
    std::string type;
    std::string targetType;
    int targetId = 0;
    std::string payload;
};

// Reads the log back for one debate, in replay order.
std::vector<MoveRow> readMoves(Database& db, int debateId) {
    std::vector<MoveRow> out;
    sqlite3_stmt* stmt = db.prepare(
        "SELECT SEQ, ACTOR_ID, TYPE, TARGET_TYPE, TARGET_ID, IFNULL(PAYLOAD,'') "
        "FROM MOVES WHERE DEBATE_ID = ? ORDER BY SEQ;");
    if (!stmt) return out;
    sqlite3_bind_int(stmt, 1, debateId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        MoveRow r;
        r.seq        = sqlite3_column_int(stmt, 0);
        r.actorId    = sqlite3_column_int(stmt, 1);
        r.type       = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        r.targetType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.targetId   = sqlite3_column_int(stmt, 4);
        r.payload    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        out.push_back(r);
    }
    sqlite3_finalize(stmt);
    return out;
}

// "1:ASSERT(claim) 2:OPPOSE(claim)" -- compact shape for whole-sequence
// assertions, so a failure message shows the actual sequence rather than
// just a count mismatch.
std::string shapeOf(const std::vector<MoveRow>& moves) {
    std::string s;
    for (const auto& m : moves) {
        if (!s.empty()) s += " ";
        s += std::to_string(m.seq) + ":" + m.type + "(" + m.targetType + ")";
    }
    return s;
}

}  // namespace

// =====================================================================
// Storage primitive
// =====================================================================
class MoveDatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = std::string("test_movedb_") + std::to_string(getpid()) + ".sqlite3";
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

    std::string db_path_;
    Database* db_ = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
};

TEST_F(MoveDatabaseTest, SeqStartsAtOneAndIncrements) {
    ASSERT_TRUE(wrapper_->moves.appendMove(1, 100, "ASSERT", "claim", 10, ""));
    ASSERT_TRUE(wrapper_->moves.appendMove(1, 100, "ASSERT", "claim", 11, ""));
    ASSERT_TRUE(wrapper_->moves.appendMove(1, 100, "RETRACT", "claim", 11, ""));

    auto moves = readMoves(*db_, 1);
    ASSERT_EQ(moves.size(), 3u);
    EXPECT_EQ(moves[0].seq, 1);
    EXPECT_EQ(moves[1].seq, 2);
    EXPECT_EQ(moves[2].seq, 3);
}

TEST_F(MoveDatabaseTest, SeqIsPerDebateNotGlobal) {
    // Interleaved across two debates: each must keep its own sequence, or
    // replaying one debate would inherit gaps from the other.
    wrapper_->moves.appendMove(1, 100, "ASSERT", "claim", 10, "");
    wrapper_->moves.appendMove(2, 100, "ASSERT", "claim", 20, "");
    wrapper_->moves.appendMove(1, 100, "ASSERT", "claim", 11, "");
    wrapper_->moves.appendMove(2, 100, "ASSERT", "claim", 21, "");

    auto d1 = readMoves(*db_, 1);
    auto d2 = readMoves(*db_, 2);
    ASSERT_EQ(d1.size(), 2u);
    ASSERT_EQ(d2.size(), 2u);
    EXPECT_EQ(d1[0].seq, 1);
    EXPECT_EQ(d1[1].seq, 2);
    EXPECT_EQ(d2[0].seq, 1);
    EXPECT_EQ(d2[1].seq, 2);
}

TEST_F(MoveDatabaseTest, SeqIsGaplessSoTheLogIsReplayable) {
    for (int i = 0; i < 25; ++i) {
        ASSERT_TRUE(wrapper_->moves.appendMove(7, 100, "ASSERT", "claim", 100 + i, ""));
    }
    auto moves = readMoves(*db_, 7);
    ASSERT_EQ(moves.size(), 25u);
    for (size_t i = 0; i < moves.size(); ++i) {
        EXPECT_EQ(moves[i].seq, static_cast<int>(i) + 1)
            << "sequence must be gapless; a hole makes replay order ambiguous";
    }
}

TEST_F(MoveDatabaseTest, RecordsTargetKindAndPayload) {
    wrapper_->moves.appendMove(1, 100, "ASSERT", "relation", 42, "{\"from_claim_id\":1}");
    auto moves = readMoves(*db_, 1);
    ASSERT_EQ(moves.size(), 1u);
    EXPECT_EQ(moves[0].targetType, "relation");
    EXPECT_EQ(moves[0].targetId, 42);
    EXPECT_EQ(moves[0].actorId, 100);
    EXPECT_EQ(moves[0].payload, "{\"from_claim_id\":1}");
}

TEST_F(MoveDatabaseTest, RefusesMovesWithNoDebate) {
    // A move that cannot be placed in a debate has no position in any
    // sequence, so it must be rejected rather than stored as an orphan.
    EXPECT_FALSE(wrapper_->moves.appendMove(0, 100, "ASSERT", "claim", 10, ""));
    EXPECT_FALSE(wrapper_->moves.appendMove(-1, 100, "ASSERT", "claim", 10, ""));
}

TEST_F(MoveDatabaseTest, ConcurrentAppendsProduceNoDuplicateOrMissingSeq) {
    // Sequence assignment happens inside a single INSERT...SELECT specifically
    // so two writers cannot both read the same MAX(SEQ). That was previously
    // an argument about SQLite semantics rather than a measurement, and the
    // reasoning is not obviously safe: Database guards individual statements,
    // but prepare() hands back a statement the caller steps outside the lock.
    // So measure it.
    constexpr int kThreads = 8;
    constexpr int kPerThread = 25;
    constexpr int kDebate = 99;

    std::atomic<int> failures{0};
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < kPerThread; ++i) {
                if (!wrapper_->moves.appendMove(kDebate, 100 + t, "ASSERT", "claim",
                                                1000 + t * kPerThread + i, "")) {
                    failures.fetch_add(1);
                }
            }
        });
    }
    for (auto& th : threads) th.join();

    EXPECT_EQ(failures.load(), 0) << "no append should fail under contention";

    auto moves = readMoves(*db_, kDebate);
    ASSERT_EQ(moves.size(), static_cast<size_t>(kThreads * kPerThread));

    std::set<int> seqs;
    for (const auto& m : moves) {
        EXPECT_TRUE(seqs.insert(m.seq).second)
            << "duplicate seq " << m.seq << " -- two writers claimed the same slot";
    }
    for (int i = 1; i <= kThreads * kPerThread; ++i) {
        EXPECT_TRUE(seqs.count(i)) << "missing seq " << i << " -- the sequence has a hole";
    }
}

TEST_F(MoveDatabaseTest, LogFailureDoesNotBreakTheOperation) {
    // The core parallel-phase guarantee. With the table gone, appending must
    // report failure rather than throw, and ordinary debate work must carry on
    // untouched.
    ASSERT_TRUE(db_->execute("DROP TABLE MOVES;"));

    EXPECT_NO_THROW({
        EXPECT_FALSE(wrapper_->moves.appendMove(1, 100, "ASSERT", "claim", 10, ""));
    });

    // The rest of the system still works with no move log present at all.
    DebateWrapper debate(*wrapper_);
    int userId = wrapper_->users.createUser("alice", {});
    ASSERT_GT(userId, 0);
    int claimId = debate.createClaim("still works", "", userId, 1);
    EXPECT_GT(claimId, 0) << "a broken move log must not stop claims being created";
    EXPECT_EQ(debate.getClaimById(claimId).sentence(), "still works");
}

// =====================================================================
// Full pipeline: forged events through DebateModerator
// =====================================================================
class MoveLogTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = std::string("test_movelog_") + std::to_string(getpid()) + ".sqlite3";
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
        _putenv_s("DB_PATH", db_path_.c_str());

        // db_ must outlive moderator_, which holds a reference to it.
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
        _putenv_s("DB_PATH", "");
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
    }

    debate_event::DebateEvent baseEvent(debate_event::EventType type) {
        debate_event::DebateEvent e;
        e.mutable_user()->set_user_id(aliceId_);
        e.mutable_user()->set_username("alice");
        e.mutable_user()->set_is_logged_in(true);
        e.set_type(type);
        return e;
    }

    void send(debate_event::DebateEvent& e) { moderator_->handleRequest(e); }

    int currentDebateId() {
        return moderator_->getDebateWrapper()
            .getUserProtobuf(aliceId_)
            .engagement().debating_info().debate_id();
    }

    int currentClaimId() {
        return moderator_->getDebateWrapper()
            .getUserProtobuf(aliceId_)
            .engagement().debating_info().current_claim().id();
    }

    // Creates a debate and enters it. Returns its id.
    int createAndEnterDebate(const std::string& topic) {
        auto create = baseEvent(debate_event::CREATE_DEBATE);
        create.mutable_create_debate()->set_debate_topic(topic);
        send(create);

        std::vector<int> ids = moderator_->getDebateWrapper().getUserDebateIds(aliceId_);
        EXPECT_FALSE(ids.empty());
        int debateId = ids.empty() ? 0 : ids.back();

        auto enter = baseEvent(debate_event::ENTER_DEBATE);
        enter.mutable_enter_debate()->set_debate_id(debateId);
        send(enter);
        return debateId;
    }

    void addChildClaim(const std::string& text) {
        auto e = baseEvent(debate_event::ADD_CHILD_CLAIM);
        e.mutable_add_child_claim()->set_claim(text);
        e.mutable_add_child_claim()->set_description("");
        send(e);
    }

    std::string db_path_;
    Database* db_ = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
    DebateModerator* moderator_ = nullptr;
    int aliceId_ = 0;
};

TEST_F(MoveLogTest, CreatingADebateLogsExactlyOneAssertForTheRootClaim) {
    // This is the regression that motivated the file: CREATE_DEBATE does not
    // move the user into the new debate, so deriving the root claim from the
    // user's engagement silently logged nothing at all.
    int debateId = createAndEnterDebate("Rent control causes high rents");

    auto moves = readMoves(*db_, debateId);
    ASSERT_EQ(moves.size(), 1u) << "shape was: " << shapeOf(moves);
    EXPECT_EQ(moves[0].type, "ASSERT");
    EXPECT_EQ(moves[0].targetType, "claim");
    EXPECT_EQ(moves[0].actorId, aliceId_);
    EXPECT_GT(moves[0].targetId, 0) << "must point at the real root claim";
}

TEST_F(MoveLogTest, AddingAChildLogsTheClaimAndTheRelation) {
    int debateId = createAndEnterDebate("Root");
    addChildClaim("Supporting evidence");

    auto moves = readMoves(*db_, debateId);
    // root ASSERT, child ASSERT, relation ASSERT -- and nothing else.
    ASSERT_EQ(moves.size(), 3u) << "shape was: " << shapeOf(moves);
    EXPECT_EQ(shapeOf(moves), "1:ASSERT(claim) 2:ASSERT(claim) 3:ASSERT(relation)");
    EXPECT_NE(moves[1].targetId, moves[0].targetId)
        << "the child must be logged, not the root a second time";
}

TEST_F(MoveLogTest, DeletingAClaimLogsRetractForTheClaimOnly) {
    int debateId = createAndEnterDebate("Root");
    addChildClaim("Doomed claim");

    int childId = 0;
    {
        auto moves = readMoves(*db_, debateId);
        ASSERT_EQ(moves.size(), 3u);
        childId = moves[1].targetId;
    }

    auto goTo = baseEvent(debate_event::GO_TO_CLAIM);
    goTo.mutable_go_to_claim()->set_claim_id(childId);
    send(goTo);

    auto del = baseEvent(debate_event::DELETE_CURRENT_STATEMENT);
    send(del);

    auto moves = readMoves(*db_, debateId);
    // Exactly one new move. Deleting the claim also destroys its PARENT_CHILD
    // relation, but that is a cascade, not a second user action -- logging it
    // would misrepresent one decision as two.
    ASSERT_EQ(moves.size(), 4u) << "shape was: " << shapeOf(moves);
    EXPECT_EQ(moves[3].type, "RETRACT");
    EXPECT_EQ(moves[3].targetType, "claim");
    EXPECT_EQ(moves[3].targetId, childId);
}

TEST_F(MoveLogTest, ChallengingLogsAssertThenOpposeAgainstTheChallengedClaim) {
    // Not reachable through the current UI, so this is the only place the
    // OPPOSE mapping is exercised at all.
    int debateId = createAndEnterDebate("Root claim");
    const int rootClaimId = currentClaimId();
    ASSERT_GT(rootClaimId, 0);

    auto start = baseEvent(debate_event::START_CHALLENGE_CLAIM);
    send(start);

    auto addTarget = baseEvent(debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
    addTarget.mutable_add_claim_to_be_challenged()->set_claim_id(rootClaimId);
    send(addTarget);

    auto open = baseEvent(debate_event::OPEN_ADD_CHALLENGE);
    send(open);

    auto submit = baseEvent(debate_event::SUBMIT_CHALLENGE_CLAIM);
    submit.mutable_submit_challenge_claim()->set_challenge_sentence("That is correlation, not cause");
    submit.mutable_submit_challenge_claim()->set_challenge_description("");
    send(submit);

    auto moves = readMoves(*db_, debateId);
    // root ASSERT, then the challenge claim ASSERT, then OPPOSE.
    ASSERT_EQ(moves.size(), 3u) << "shape was: " << shapeOf(moves);
    EXPECT_EQ(shapeOf(moves), "1:ASSERT(claim) 2:ASSERT(claim) 3:OPPOSE(claim)");

    // The golden rule in miniature: OPPOSE names the claim under attack, not
    // the relation carrying the attack.
    EXPECT_EQ(moves[2].targetId, rootClaimId)
        << "OPPOSE must target the challenged claim";
    EXPECT_NE(moves[2].payload.find("relation_id"), std::string::npos)
        << "the challenge relation belongs in the payload";
}

TEST_F(MoveLogTest, ConcedingLogsAgainstTheChallengedClaim) {
    // CONCEDE had never been observed firing anywhere -- not in the UI (the
    // challenge view is unreachable) and not in any test. This is the first
    // time it is actually exercised.
    int debateId = createAndEnterDebate("Root claim");
    const int rootClaimId = currentClaimId();

    auto start = baseEvent(debate_event::START_CHALLENGE_CLAIM);
    send(start);
    auto addTarget = baseEvent(debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
    addTarget.mutable_add_claim_to_be_challenged()->set_claim_id(rootClaimId);
    send(addTarget);
    auto open = baseEvent(debate_event::OPEN_ADD_CHALLENGE);
    send(open);
    auto submit = baseEvent(debate_event::SUBMIT_CHALLENGE_CLAIM);
    submit.mutable_submit_challenge_claim()->set_challenge_sentence("Correlation, not cause");
    submit.mutable_submit_challenge_claim()->set_challenge_description("");
    send(submit);

    // SubmitChallengeClaim leaves the user standing on the challenge claim, so
    // its outgoing CHALLENGE link is the one to concede to.
    const int challengeClaimId = currentClaimId();
    const int challengeLinkId =
        moderator_->getDebateWrapper().findOutgoingChallengeLink(challengeClaimId).link().id();
    ASSERT_GT(challengeLinkId, 0);

    const size_t before = readMoves(*db_, debateId).size();

    auto concede = baseEvent(debate_event::CONCEDE_CHALLENGE);
    concede.mutable_concede_challenge()->set_challenge_link_id(challengeLinkId);
    send(concede);

    auto moves = readMoves(*db_, debateId);
    ASSERT_EQ(moves.size(), before + 1)
        << "conceding is one decision, not one per cascaded status change; shape was: "
        << shapeOf(moves);

    const auto& m = moves.back();
    EXPECT_EQ(m.type, "CONCEDE");
    EXPECT_EQ(m.targetType, "claim");
    EXPECT_EQ(m.targetId, rootClaimId)
        << "CONCEDE names the claim being given up, not the challenge";
    EXPECT_NE(m.payload.find("challenge_relation_id"), std::string::npos);
}

TEST_F(MoveLogTest, DeletingAClaimWithChildrenAndLinksLogsExactlyOneRetract) {
    // Deleting a claim tears down every link touching it, and can orphan a
    // whole subtree. None of that is a separate user decision, so none of it
    // may appear in the log as one.
    int debateId = createAndEnterDebate("Root");
    addChildClaim("Middle claim");

    int middleId = 0;
    {
        auto moves = readMoves(*db_, debateId);
        ASSERT_EQ(moves.size(), 3u);
        middleId = moves[1].targetId;
    }

    // Give the middle claim a child of its own, so deleting it destroys two
    // relations and orphans a claim.
    auto goToMiddle = baseEvent(debate_event::GO_TO_CLAIM);
    goToMiddle.mutable_go_to_claim()->set_claim_id(middleId);
    send(goToMiddle);
    addChildClaim("Grandchild claim");

    const size_t before = readMoves(*db_, debateId).size();
    ASSERT_EQ(before, 5u) << "root + middle + rel + grandchild + rel";

    // Delete the middle claim, which has both a parent link and a child link.
    auto goAgain = baseEvent(debate_event::GO_TO_CLAIM);
    goAgain.mutable_go_to_claim()->set_claim_id(middleId);
    send(goAgain);
    auto del = baseEvent(debate_event::DELETE_CURRENT_STATEMENT);
    send(del);

    auto moves = readMoves(*db_, debateId);
    EXPECT_EQ(moves.size(), before + 1)
        << "exactly one RETRACT, no cascade rows; shape was: " << shapeOf(moves);
    EXPECT_EQ(moves.back().type, "RETRACT");
    EXPECT_EQ(moves.back().targetId, middleId);
}

TEST_F(MoveLogTest, NavigationAndUiStateProduceNoMoves) {
    // Roughly seventy of the eighty-odd event types are UI state. If any of
    // them started logging, the log would fill with noise that replay would
    // have to interpret as user decisions.
    int debateId = createAndEnterDebate("Root");
    const size_t before = readMoves(*db_, debateId).size();

    auto open = baseEvent(debate_event::OPEN_ADD_CHILD_CLAIM);
    send(open);
    auto close = baseEvent(debate_event::CLOSE_ADD_CHILD_CLAIM);
    send(close);
    auto overview = baseEvent(debate_event::GO_TO_OVERVIEW);
    send(overview);
    auto home = baseEvent(debate_event::GO_HOME);
    send(home);

    EXPECT_EQ(readMoves(*db_, debateId).size(), before)
        << "UI state and navigation are not moves";
}

TEST_F(MoveLogTest, SeparateDebatesKeepIndependentSequences) {
    int first = createAndEnterDebate("First debate");
    addChildClaim("child in first");

    int second = createAndEnterDebate("Second debate");
    addChildClaim("child in second");

    auto a = readMoves(*db_, first);
    auto b = readMoves(*db_, second);
    ASSERT_EQ(a.size(), 3u) << "shape was: " << shapeOf(a);
    ASSERT_EQ(b.size(), 3u) << "shape was: " << shapeOf(b);
    EXPECT_EQ(a[0].seq, 1);
    EXPECT_EQ(b[0].seq, 1) << "the second debate must not inherit the first's sequence";
    EXPECT_NE(first, second);
}
