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
#include <iostream>
#include <tuple>
#include <cstdio>
#include <cstdlib>

#include "debate.pb.h"
#include "debate_event.pb.h"
#include "user.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "utils/DebateWrapper.h"
#include "debateModerator/DebateModerator.h"
#include "statusEngine/StatusEngine.h"
#include "debateModerator/buildResponse/debatePageResponse/BuildCollection.h"
#include <optional>

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

    // Events are attributed per user, so anything multi-user has to build them
    // for a named actor rather than the fixture's default.
    debate_event::DebateEvent eventFor(int userId, const std::string& username,
                                       debate_event::EventType type) {
        debate_event::DebateEvent e;
        e.mutable_user()->set_user_id(userId);
        e.mutable_user()->set_username(username);
        e.mutable_user()->set_is_logged_in(true);
        e.set_type(type);
        return e;
    }

    debate_event::DebateEvent baseEvent(debate_event::EventType type) {
        return eventFor(aliceId_, "alice", type);
    }

    void send(debate_event::DebateEvent& e) { moderator_->handleRequest(e); }

    int currentClaimIdFor(int userId) {
        return moderator_->getDebateWrapper()
            .getUserProtobuf(userId)
            .engagement().debating_info().current_claim().id();
    }

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

// =====================================================================
// Old engine vs new engine, on the case that actually diverges
// =====================================================================
//
// Reported symptom: after challenging a claim, the challenged node shows
// UNDETERMINED and its parent shows True. This test builds exactly that
// scenario and prints both engines' answers so the difference is visible
// rather than argued about.
//
// It deliberately asserts only what each engine DOES, not which is right --
// that is a design question, and encoding an opinion here would just bake in
// whichever answer was written first.

namespace {

const char* legacyName(debate::ClaimStatus s) {
    switch (s) {
        case debate::ClaimStatus::TRUE_CLAIM:  return "TRUE_CLAIM";
        case debate::ClaimStatus::FALSE_CLAIM: return "FALSE_CLAIM";
        default:                               return "UNDETERMINED";
    }
}

}  // namespace

TEST_F(MoveLogTest, ChallengingOnesOwnReasonRefutesItAndSinksTheRoot) {
    int debateId = createAndEnterDebate("Root claim");
    const int rootId = currentClaimId();
    ASSERT_GT(rootId, 0);

    addChildClaim("Supporting evidence");
    int childId = 0;
    for (const auto& m : readMoves(*db_, debateId)) {
        if (m.type == "ASSERT" && m.targetType == "claim" && m.targetId != rootId) {
            childId = m.targetId;
        }
    }
    ASSERT_GT(childId, 0);

    // Challenge the CHILD -- the claim that supports the root.
    auto goToChild = baseEvent(debate_event::GO_TO_CLAIM);
    goToChild.mutable_go_to_claim()->set_claim_id(childId);
    send(goToChild);

    auto start = baseEvent(debate_event::START_CHALLENGE_CLAIM);
    send(start);
    auto addTarget = baseEvent(debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
    addTarget.mutable_add_claim_to_be_challenged()->set_claim_id(childId);
    send(addTarget);
    auto open = baseEvent(debate_event::OPEN_ADD_CHALLENGE);
    send(open);
    auto submit = baseEvent(debate_event::SUBMIT_CHALLENGE_CLAIM);
    submit.mutable_submit_challenge_claim()->set_challenge_sentence("That does not follow");
    submit.mutable_submit_challenge_claim()->set_challenge_description("");
    send(submit);

    const int challengeClaimId = currentClaimId();
    ASSERT_GT(challengeClaimId, 0);
    ASSERT_NE(challengeClaimId, childId);

    DebateWrapper& dw = moderator_->getDebateWrapper();

    std::vector<debate::Claim> claims;
    for (int id : {rootId, childId, challengeClaimId}) {
        claims.push_back(dw.getClaimById(id));
    }
    std::vector<debate::Relationship> relations;
    for (const auto& row : dw.getLinksForDebate(debateId)) {
        relations.push_back(dw.getLinkById(std::get<0>(row)));
    }
    std::vector<debate::Move> moves = dw.getMovesForDebate(debateId);
    auto computed = StatusEngine::computeLegacyStatuses(claims, relations, moves,
                                                       {{aliceId_, "alice"}});

    auto computedFor = [&](int claimId) {
        auto uit = computed.by_user.find("alice");
        if (uit == computed.by_user.end()) return debate::ClaimStatus::UNDETERMINED;
        auto cit = uit->second.find(claimId);
        return cit == uit->second.end() ? debate::ClaimStatus::UNDETERMINED : cit->second;
    };

    // Alice's own challenge is TRUE for her, so her reason is FALSE, and a
    // FALSE reason on a COLLAPSE edge sinks the root. These are the values
    // the stored engine used to write; the log now yields them alone.
    EXPECT_EQ(computedFor(challengeClaimId), debate::ClaimStatus::TRUE_CLAIM) << "challenge claim";
    EXPECT_EQ(computedFor(childId), debate::ClaimStatus::FALSE_CLAIM) << "challenged child";
    EXPECT_EQ(computedFor(rootId), debate::ClaimStatus::FALSE_CLAIM) << "root";
}

// Conceding is the seed with the most moving parts -- the handler writes four
// separate groups of statuses, and all of them have to be rebuilt from a single
// CONCEDE row plus its payload. It is also unreachable from the current UI, so
// this test is the only thing holding that path honest.
TEST_F(MoveLogTest, ConcedingAChallengeOnTheRootMakesItFalseForTheConceder) {
    int debateId = createAndEnterDebate("Root claim");
    const int rootId = currentClaimId();
    ASSERT_GT(rootId, 0);

    auto start = baseEvent(debate_event::START_CHALLENGE_CLAIM);
    send(start);
    auto addTarget = baseEvent(debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
    addTarget.mutable_add_claim_to_be_challenged()->set_claim_id(rootId);
    send(addTarget);
    auto open = baseEvent(debate_event::OPEN_ADD_CHALLENGE);
    send(open);
    auto submit = baseEvent(debate_event::SUBMIT_CHALLENGE_CLAIM);
    submit.mutable_submit_challenge_claim()->set_challenge_sentence("Correlation, not cause");
    submit.mutable_submit_challenge_claim()->set_challenge_description("");
    send(submit);

    const int challengeClaimId = currentClaimId();
    ASSERT_GT(challengeClaimId, 0);

    DebateWrapper& dw = moderator_->getDebateWrapper();
    const int challengeLinkId = dw.findOutgoingChallengeLink(challengeClaimId).link().id();
    ASSERT_GT(challengeLinkId, 0);

    auto concede = baseEvent(debate_event::CONCEDE_CHALLENGE);
    concede.mutable_concede_challenge()->set_challenge_link_id(challengeLinkId);
    send(concede);

    std::vector<debate::Claim> claims;
    for (int id : {rootId, challengeClaimId}) {
        claims.push_back(dw.getClaimById(id));
    }
    std::vector<debate::Relationship> relations;
    for (const auto& row : dw.getLinksForDebate(debateId)) {
        relations.push_back(dw.getLinkById(std::get<0>(row)));
    }
    std::vector<debate::Move> moves = dw.getMovesForDebate(debateId);
    auto computed = StatusEngine::computeLegacyStatuses(claims, relations, moves,
                                                       {{aliceId_, "alice"}});

    auto computedFor = [&](int claimId) {
        auto uit = computed.by_user.find("alice");
        if (uit == computed.by_user.end()) return debate::ClaimStatus::UNDETERMINED;
        auto cit = uit->second.find(claimId);
        return cit == uit->second.end() ? debate::ClaimStatus::UNDETERMINED : cit->second;
    };

    EXPECT_EQ(computedFor(challengeClaimId), debate::ClaimStatus::TRUE_CLAIM) << "challenge claim";
    EXPECT_EQ(computedFor(rootId), debate::ClaimStatus::FALSE_CLAIM) << "conceded claim";
}

// Two real users, opposed, with the concession coming from the other side.
//
// This is the case the single-user tests cannot reach. The stored model is
// per-user, so parity only means something once two people hold DIFFERENT
// views of the same claim -- with one author the whole matrix is one column and
// a rule that mixed users up would still pass. Here alice asserts, bob attacks,
// and alice concedes, so every seed rule fires for a different actor than the
// one that created the claim.
//
// It also compares the ENTIRE matrix rather than hand-picked cells: every claim
// against every user known to either side. A rule that invents an opinion for
// someone who holds none shows up here and nowhere else.
TEST_F(MoveLogTest, TwoUserChallengeAndConcede_FullMatrix) {
    const int bobId = moderator_->createUserIfNotExist("bob");
    ASSERT_GT(bobId, 0);
    ASSERT_NE(bobId, aliceId_);

    // --- alice builds her case ---
    const int debateId = createAndEnterDebate("Nuclear power is the cheapest clean energy");
    const int rootId = currentClaimId();
    ASSERT_GT(rootId, 0);

    addChildClaim("Capacity factor is over 90 percent");
    int aliceChildId = 0;
    for (const auto& m : readMoves(*db_, debateId)) {
        if (m.type == "ASSERT" && m.targetType == "claim" && m.targetId != rootId) {
            aliceChildId = m.targetId;
        }
    }
    ASSERT_GT(aliceChildId, 0);

    // --- bob joins and attacks alice's evidence ---
    auto join = eventFor(bobId, "bob", debate_event::JOIN_DEBATE);
    join.mutable_join_debate()->set_debate_id(debateId);
    send(join);

    auto bobEnter = eventFor(bobId, "bob", debate_event::ENTER_DEBATE);
    bobEnter.mutable_enter_debate()->set_debate_id(debateId);
    send(bobEnter);

    auto bobGoto = eventFor(bobId, "bob", debate_event::GO_TO_CLAIM);
    bobGoto.mutable_go_to_claim()->set_claim_id(aliceChildId);
    send(bobGoto);

    auto bobStart = eventFor(bobId, "bob", debate_event::START_CHALLENGE_CLAIM);
    send(bobStart);
    auto bobTarget = eventFor(bobId, "bob", debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
    bobTarget.mutable_add_claim_to_be_challenged()->set_claim_id(aliceChildId);
    send(bobTarget);
    auto bobOpen = eventFor(bobId, "bob", debate_event::OPEN_ADD_CHALLENGE);
    send(bobOpen);
    auto bobSubmit = eventFor(bobId, "bob", debate_event::SUBMIT_CHALLENGE_CLAIM);
    bobSubmit.mutable_submit_challenge_claim()->set_challenge_sentence(
        "Capacity factor ignores construction cost");
    bobSubmit.mutable_submit_challenge_claim()->set_challenge_description("");
    send(bobSubmit);

    const int bobChallengeId = currentClaimIdFor(bobId);
    ASSERT_GT(bobChallengeId, 0);
    ASSERT_NE(bobChallengeId, aliceChildId);

    DebateWrapper& dw = moderator_->getDebateWrapper();
    const std::vector<int> claimIds = {rootId, aliceChildId, bobChallengeId};
    const std::set<std::string> users{"alice", "bob"};

    // Checks the entire matrix -- every claim against every user -- against
    // what is expected at that moment, keyed (claim, user).
    using Expected = std::map<std::pair<int, std::string>, debate::ClaimStatus>;
    auto checkMatrix = [&](const char* moment, const Expected& expected) {
        std::vector<debate::Claim> claims;
        for (int id : claimIds) {
            claims.push_back(dw.getClaimById(id));
        }
        std::vector<debate::Relationship> relations;
        for (const auto& row : dw.getLinksForDebate(debateId)) {
            relations.push_back(dw.getLinkById(std::get<0>(row)));
        }
        const std::vector<debate::Move> moves = dw.getMovesForDebate(debateId);

        auto computed = StatusEngine::computeLegacyStatuses(
            claims, relations, moves, {{aliceId_, "alice"}, {bobId, "bob"}});

        // Both users must actually appear, or parity would be satisfied
        // trivially by an engine that produced nothing at all.
        EXPECT_TRUE(computed.by_user.count("alice")) << moment << ": alice holds no view";
        EXPECT_TRUE(computed.by_user.count("bob")) << moment << ": bob holds no view";

        auto computedFor = [&](int claimId, const std::string& user) {
            auto uit = computed.by_user.find(user);
            if (uit == computed.by_user.end()) return debate::ClaimStatus::UNDETERMINED;
            auto cit = uit->second.find(claimId);
            return cit == uit->second.end() ? debate::ClaimStatus::UNDETERMINED : cit->second;
        };

        for (int id : claimIds) {
            for (const std::string& u : users) {
                EXPECT_EQ(computedFor(id, u), expected.at({id, u}))
                    << moment << ": claim " << id << " as seen by " << u;
            }
        }
    };

    // Moment 1: bob has attacked, alice has not answered. The two columns
    // genuinely diverge here, so this is where a rule that confused one
    // user's view for another's would show up.
    checkMatrix("after bob's challenge, before alice answers", {
        {{rootId, "alice"}, debate::ClaimStatus::TRUE_CLAIM},
        {{rootId, "bob"}, debate::ClaimStatus::FALSE_CLAIM},
        {{aliceChildId, "alice"}, debate::ClaimStatus::TRUE_CLAIM},
        {{aliceChildId, "bob"}, debate::ClaimStatus::FALSE_CLAIM},
        {{bobChallengeId, "alice"}, debate::ClaimStatus::UNDETERMINED},
        {{bobChallengeId, "bob"}, debate::ClaimStatus::TRUE_CLAIM},
    });

    // --- alice concedes bob's challenge ---
    const int challengeLinkId = dw.findOutgoingChallengeLink(bobChallengeId).link().id();
    ASSERT_GT(challengeLinkId, 0);
    auto concede = baseEvent(debate_event::CONCEDE_CHALLENGE);
    concede.mutable_concede_challenge()->set_challenge_link_id(challengeLinkId);
    send(concede);

    // Moment 2: conceding means alice adopts bob's view, so the columns
    // converge. Convergence is the correct outcome, not a weaker test -- the
    // divergence was already checked above.
    checkMatrix("after alice concedes", {
        {{rootId, "alice"}, debate::ClaimStatus::FALSE_CLAIM},
        {{rootId, "bob"}, debate::ClaimStatus::FALSE_CLAIM},
        {{aliceChildId, "alice"}, debate::ClaimStatus::FALSE_CLAIM},
        {{aliceChildId, "bob"}, debate::ClaimStatus::FALSE_CLAIM},
        {{bobChallengeId, "alice"}, debate::ClaimStatus::TRUE_CLAIM},
        {{bobChallengeId, "bob"}, debate::ClaimStatus::TRUE_CLAIM},
    });
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

TEST_F(MoveLogTest, ChallengingALinkLogsOpposeAgainstTheRelation) {
    // "Your evidence is real but it does not prove that" attacks the LINK. The
    // OPPOSE then names the relation, not a claim, and the engine seeds no
    // claim FALSE for it -- which is the whole difference between undercutting
    // an argument and refuting a claim.
    int debateId = createAndEnterDebate("Root claim");
    const int rootClaimId = currentClaimId();
    addChildClaim("Crash deaths fell 23 percent");

    auto goTo = baseEvent(debate_event::GO_TO_CLAIM);
    goTo.mutable_go_to_claim()->set_claim_id(rootClaimId);
    send(goTo);

    const std::vector<int> links = moderator_->getDebateWrapper().findLinksUnder(rootClaimId);
    ASSERT_EQ(links.size(), 1u);

    auto start = baseEvent(debate_event::START_CHALLENGE_CLAIM);
    send(start);
    auto addLink = baseEvent(debate_event::ADD_LINK_TO_BE_CHALLENGED);
    addLink.mutable_add_link_to_be_challenged()->set_link_id(links[0]);
    send(addLink);
    auto open = baseEvent(debate_event::OPEN_ADD_CHALLENGE);
    send(open);
    auto submit = baseEvent(debate_event::SUBMIT_CHALLENGE_CLAIM);
    submit.mutable_submit_challenge_claim()->set_challenge_sentence("Those boroughs were already getting safer");
    submit.mutable_submit_challenge_claim()->set_challenge_description("");
    send(submit);

    auto moves = readMoves(*db_, debateId);
    ASSERT_GE(moves.size(), 2u) << "shape was: " << shapeOf(moves);
    const MoveRow& last = moves.back();
    EXPECT_EQ(last.type, "OPPOSE");
    EXPECT_EQ(last.targetType, "relation") << shapeOf(moves);
    EXPECT_EQ(last.targetId, links[0]) << "the OPPOSE names the attacked relation";
    EXPECT_NE(last.payload.find("challenging_claim_id"), std::string::npos);
}

// =====================================================================
// Undercuts, as the PAGE sees them.
//
// Every status on screen comes from replaying the move log (BuildCollection
// applies StatusEngine's result), so these drive real events through the
// moderator -- which is what writes the log -- and read the replay back.
// Calling handlers directly would skip the log and test the wrong engine.
// =====================================================================
class UndercutFlowTest : public MoveLogTest {
protected:
    DebateWrapper& dw() { return moderator_->getDebateWrapper(); }

    int userNamed(const std::string& name) {
        const int id = moderator_->createUserIfNotExist(name);
        EXPECT_GT(id, 0);
        return id;
    }
    void joinAndEnter(int userId, const std::string& name, int debateId) {
        auto join = eventFor(userId, name, debate_event::JOIN_DEBATE);
        join.mutable_join_debate()->set_debate_id(debateId);
        send(join);
        auto enter = eventFor(userId, name, debate_event::ENTER_DEBATE);
        enter.mutable_enter_debate()->set_debate_id(debateId);
        send(enter);
    }
    void goTo(int userId, const std::string& name, int claimId) {
        auto e = eventFor(userId, name, debate_event::GO_TO_CLAIM);
        e.mutable_go_to_claim()->set_claim_id(claimId);
        send(e);
    }
    // Adds `text` under `parent` as `name`; returns the new claim's id.
    int support(int userId, const std::string& name, int parent, const std::string& text) {
        goTo(userId, name, parent);
        const std::vector<int> before = dw().findChildrenIds(parent);
        auto e = eventFor(userId, name, debate_event::ADD_CHILD_CLAIM);
        e.mutable_add_child_claim()->set_claim(text);
        e.mutable_add_child_claim()->set_description("");
        send(e);
        for (int id : dw().findChildrenIds(parent)) {
            if (std::find(before.begin(), before.end(), id) == before.end()) return id;
        }
        return 0;
    }
    // Challenges a CLAIM; returns the challenge claim's id.
    int rebut(int userId, const std::string& name, int claimId, const std::string& text) {
        goTo(userId, name, claimId);
        auto start = eventFor(userId, name, debate_event::START_CHALLENGE_CLAIM);
        send(start);
        auto target = eventFor(userId, name, debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
        target.mutable_add_claim_to_be_challenged()->set_claim_id(claimId);
        send(target);
        auto open = eventFor(userId, name, debate_event::OPEN_ADD_CHALLENGE);
        send(open);
        auto submit = eventFor(userId, name, debate_event::SUBMIT_CHALLENGE_CLAIM);
        submit.mutable_submit_challenge_claim()->set_challenge_sentence(text);
        submit.mutable_submit_challenge_claim()->set_challenge_description("");
        send(submit);
        return currentClaimIdFor(userId);
    }
    // Challenges a LINK (an argument); `standingOn` is the claim the argument
    // is for, where the user must be for the modal to find the link.
    int undercut(int userId, const std::string& name, int standingOn, int linkId, const std::string& text) {
        goTo(userId, name, standingOn);
        auto start = eventFor(userId, name, debate_event::START_CHALLENGE_CLAIM);
        send(start);
        auto target = eventFor(userId, name, debate_event::ADD_LINK_TO_BE_CHALLENGED);
        target.mutable_add_link_to_be_challenged()->set_link_id(linkId);
        send(target);
        auto open = eventFor(userId, name, debate_event::OPEN_ADD_CHALLENGE);
        send(open);
        auto submit = eventFor(userId, name, debate_event::SUBMIT_CHALLENGE_CLAIM);
        submit.mutable_submit_challenge_claim()->set_challenge_sentence(text);
        submit.mutable_submit_challenge_claim()->set_challenge_description("");
        send(submit);
        return currentClaimIdFor(userId);
    }
    void concede(int userId, const std::string& name, int challengeLinkId) {
        auto e = eventFor(userId, name, debate_event::CONCEDE_CHALLENGE);
        e.mutable_concede_challenge()->set_challenge_link_id(challengeLinkId);
        send(e);
    }
    int linkBetween(int parent, int child) {
        for (int linkId : dw().findLinksUnder(parent)) {
            const debate::Relationship::Link l = dw().getLinkById(linkId).link();
            if (l.connect_from() == parent && l.connect_to() == child) return linkId;
        }
        return 0;
    }
    // What the page shows `user` for `claimId`: the replayed status, or none.
    std::optional<debate::ClaimStatus> shown(int debateId, int claimId, const std::string& user) {
        const debate::Collection c = BuildCollection::BuildForDebateAndUsers(
            debateId, dw().findUsersInDebate(debateId), dw());
        auto claimIt = c.claims_by_id().find(claimId);
        if (claimIt == c.claims_by_id().end()) return std::nullopt;
        auto it = claimIt->second.user_statuses().find(user);
        if (it == claimIt->second.user_statuses().end()) return std::nullopt;
        return it->second;
    }
    static bool undetermined(const std::optional<debate::ClaimStatus>& s) {
        return !s.has_value() || *s == debate::ClaimStatus::UNDETERMINED;
    }
    static std::string show(const std::optional<debate::ClaimStatus>& s) {
        if (!s) return "(none)";
        return *s == debate::ClaimStatus::TRUE_CLAIM ? "TRUE" : *s == debate::ClaimStatus::FALSE_CLAIM ? "FALSE" : "UNDETERMINED";
    }
};

// Cut one of two reasons: the claim stands on the other. Cut both: no reason
// is left, and no reason is not a refutation -- Undetermined, never False.
TEST_F(UndercutFlowTest, UndercutOneOfTwoReasonsAndTheClaimStandsOnTheOther) {
    const int debateId = createAndEnterDebate("Speed limits save lives");
    const int root = currentClaimId();
    const int bob = userNamed("bob");
    joinAndEnter(bob, "bob", debateId);

    // Bob supplies BOTH reasons, so for Bob the root is True by Rule B.
    const int deaths = support(bob, "bob", root, "Crash deaths fell 23 percent");
    const int stopping = support(bob, "bob", root, "Stopping distance halves at 20mph");
    ASSERT_GT(deaths, 0);
    ASSERT_GT(stopping, 0);
    ASSERT_EQ(show(shown(debateId, root, "bob")), "TRUE");

    // Bob undercuts his first reason's link.
    undercut(bob, "bob", root, linkBetween(root, deaths), "Those boroughs were already getting safer");
    EXPECT_EQ(show(shown(debateId, root, "bob")), "TRUE") << "the other reason still holds the root up";
    EXPECT_EQ(show(shown(debateId, deaths, "bob")), "TRUE") << "the undercut evidence itself is untouched";

    // And the second. Now nothing feeds the root for Bob.
    undercut(bob, "bob", root, linkBetween(root, stopping), "Stopping distance is not what kills");
    const auto rootForBob = shown(debateId, root, "bob");
    EXPECT_TRUE(undetermined(rootForBob)) << "no reason left is not a refutation; got " << show(rootForBob);
    // Alice, who never held Bob's undercuts true, is unaffected.
    EXPECT_EQ(show(shown(debateId, root, "alice")), "TRUE");
}

// A challenge's own link can be undercut. For whoever holds that undercut, the
// challenge stops attacking its target -- even though the challenge claim
// itself stays true for them.
TEST_F(UndercutFlowTest, UndercuttingAChallengesLinkDisarmsTheChallenge) {
    const int debateId = createAndEnterDebate("Speed limits save lives");
    const int root = currentClaimId();
    const int bob = userNamed("bob");
    const int carol = userNamed("carol");
    joinAndEnter(bob, "bob", debateId);
    joinAndEnter(carol, "carol", debateId);

    // Bob challenges the root: for Bob it is False by his own assertion.
    const int challenge = rebut(bob, "bob", root, "Lower limits just move traffic to other streets");
    ASSERT_GT(challenge, 0);
    ASSERT_EQ(show(shown(debateId, root, "bob")), "FALSE");

    // Carol supports Bob's challenge: for Carol it is True (Rule B), so the
    // root is False for her too (Rule D) -- derived, not asserted.
    ASSERT_GT(support(carol, "carol", challenge, "Traffic counts rose on the parallel avenue"), 0);
    ASSERT_EQ(show(shown(debateId, challenge, "carol")), "TRUE");
    ASSERT_EQ(show(shown(debateId, root, "carol")), "FALSE");

    // Carol undercuts the CHALLENGE's link: true, but it does not bear on the root.
    const int challengeLink = dw().findOutgoingChallengeLink(challenge).link().id();
    ASSERT_GT(challengeLink, 0);
    undercut(carol, "carol", root, challengeLink, "Displaced traffic is slower too, so lives are still saved");

    EXPECT_EQ(show(shown(debateId, challenge, "carol")), "TRUE") << "the challenge claim is still true for Carol";
    EXPECT_TRUE(undetermined(shown(debateId, root, "carol")))
        << "but it no longer attacks the root for her; got " << show(shown(debateId, root, "carol"));
    // Bob ASSERTED the root false when he challenged it; Carol's undercut is her view, not his.
    EXPECT_EQ(show(shown(debateId, root, "bob")), "FALSE");
}

// An undercut only bites while its claim is true for you. Get it rebutted and
// concede, and the link you cut comes back to life.
TEST_F(UndercutFlowTest, AConcededUndercutRestoresTheLink) {
    const int debateId = createAndEnterDebate("Speed limits save lives");
    const int root = currentClaimId();
    const int bob = userNamed("bob");
    joinAndEnter(bob, "bob", debateId);

    // Bob supplies the one reason: the root is True for Bob...
    const int deaths = support(bob, "bob", root, "Crash deaths fell 23 percent");
    ASSERT_GT(deaths, 0);
    ASSERT_EQ(show(shown(debateId, root, "bob")), "TRUE");

    // ...then has second thoughts and undercuts his own link.
    const int cut = undercut(bob, "bob", root, linkBetween(root, deaths), "Those boroughs were already getting safer");
    ASSERT_GT(cut, 0);
    ASSERT_TRUE(undetermined(shown(debateId, root, "bob"))) << "the reason is cut; got " << show(shown(debateId, root, "bob"));

    // Alice rebuts the undercut claim itself...
    const int rebuttal = rebut(aliceId_, "alice", cut, "The safety trend was flat before the limit changed");
    const int rebuttalLink = dw().findOutgoingChallengeLink(rebuttal).link().id();
    ASSERT_GT(rebuttalLink, 0);

    // ...and Bob concedes. His undercut is False for him now, so it cuts
    // nothing: the link is back, and so is his reason.
    concede(bob, "bob", rebuttalLink);
    EXPECT_EQ(show(shown(debateId, cut, "bob")), "FALSE") << "the conceded undercut is false for Bob";
    EXPECT_EQ(show(shown(debateId, root, "bob")), "TRUE") << "the link is live again and the root stands on its reason";
}


// ---------------------------------------------------------------------------
// Voice: a claim entered on someone else's behalf is held by THEM.
// ---------------------------------------------------------------------------
TEST_F(MoveLogTest, AChildWithAVoiceIsTrueForTheVoiceNotTheEntrant) {
    const int debateId = createAndEnterDebate("The bridge should close");
    const int rootId = currentClaimId();

    auto e = baseEvent(debate_event::ADD_CHILD_CLAIM);
    e.mutable_add_child_claim()->set_claim("The bridge is unsafe");
    e.mutable_add_child_claim()->set_voice("  the reporter ");
    send(e);

    const int reporterId = wrapper_->users.getUserId("the reporter");
    ASSERT_GT(reporterId, 0) << "naming a voice creates a user for it, trimmed";

    int childId = 0;
    for (const auto& m : readMoves(*db_, debateId)) {
        if (m.type == "ASSERT" && m.targetType == "claim" && m.targetId != rootId) childId = m.targetId;
    }
    ASSERT_GT(childId, 0);

    DebateWrapper& dw = moderator_->getDebateWrapper();
    const debate::Claim child = dw.getClaimById(childId);
    EXPECT_EQ(child.voice_id(), reporterId);
    EXPECT_EQ(child.creator_id(), aliceId_) << "the entrant still owns the row";

    // The replay agrees: the ASSERT's actor is alice, the claim's holder is not.
    std::vector<debate::Claim> claims{dw.getClaimById(rootId), child};
    std::vector<debate::Relationship> relations;
    for (const auto& row : dw.getLinksForDebate(debateId)) {
        relations.push_back(dw.getLinkById(std::get<0>(row)));
    }
    const auto computed = StatusEngine::computeLegacyStatuses(
        claims, relations, dw.getMovesForDebate(debateId),
        {{aliceId_, "alice"}, {reporterId, "the reporter"}});
    auto view = [&](const std::string& user, int claimId) {
        auto u = computed.by_user.find(user);
        if (u == computed.by_user.end()) return debate::ClaimStatus::UNDETERMINED;
        auto c = u->second.find(claimId);
        return c == u->second.end() ? debate::ClaimStatus::UNDETERMINED : c->second;
    };
    EXPECT_EQ(view("the reporter", childId), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(view("alice", childId), debate::ClaimStatus::UNDETERMINED);
}

TEST_F(MoveLogTest, AChallengeWithAVoiceRecordsTheVoicesOpinionNotTheEntrants) {
    const int debateId = createAndEnterDebate("The bridge should close");
    const int rootId = currentClaimId();

    // Paul, entered by alice, says the root is wrong.
    auto start = baseEvent(debate_event::START_CHALLENGE_CLAIM);
    send(start);
    auto addTarget = baseEvent(debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
    addTarget.mutable_add_claim_to_be_challenged()->set_claim_id(rootId);
    send(addTarget);
    auto open = baseEvent(debate_event::OPEN_ADD_CHALLENGE);
    send(open);
    auto submit = baseEvent(debate_event::SUBMIT_CHALLENGE_CLAIM);
    submit.mutable_submit_challenge_claim()->set_challenge_sentence("The bridge passed inspection");
    submit.mutable_submit_challenge_claim()->set_voice("Paul");
    send(submit);

    const int paulId = wrapper_->users.getUserId("Paul");
    ASSERT_GT(paulId, 0);
    const int challengeId = currentClaimId();
    ASSERT_NE(challengeId, rootId);

    DebateWrapper& dw = moderator_->getDebateWrapper();
    const debate::Claim root = dw.getClaimById(rootId);
    const debate::Claim challenge = dw.getClaimById(challengeId);
    EXPECT_EQ(challenge.voice_id(), paulId);

    // The OPPOSE names the voice, so the replay lands the FALSE on Paul.
    bool opposeNamesPaul = false;
    for (const auto& m : readMoves(*db_, debateId)) {
        if (m.type == "OPPOSE") {
            opposeNamesPaul = m.payload.find("\"voice_id\":" + std::to_string(paulId)) != std::string::npos;
        }
    }
    EXPECT_TRUE(opposeNamesPaul);

    std::vector<debate::Relationship> relations;
    for (const auto& row : dw.getLinksForDebate(debateId)) {
        relations.push_back(dw.getLinkById(std::get<0>(row)));
    }
    const auto computed = StatusEngine::computeLegacyStatuses(
        {root, challenge}, relations, dw.getMovesForDebate(debateId),
        {{aliceId_, "alice"}, {paulId, "Paul"}});
    auto view = [&](const std::string& user, int claimId) {
        auto u = computed.by_user.find(user);
        if (u == computed.by_user.end()) return debate::ClaimStatus::UNDETERMINED;
        auto c = u->second.find(claimId);
        return c == u->second.end() ? debate::ClaimStatus::UNDETERMINED : c->second;
    };
    EXPECT_EQ(view("Paul", rootId), debate::ClaimStatus::FALSE_CLAIM);
    EXPECT_EQ(view("Paul", challengeId), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(view("alice", rootId), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(view("alice", challengeId), debate::ClaimStatus::UNDETERMINED);
}


// ---------------------------------------------------------------------------
// Sources: where a claim was taken from rides on the claim.
// ---------------------------------------------------------------------------
TEST_F(MoveLogTest, AChildCarriesItsSourceAndAChildWithoutOneCarriesNone) {
    const int debateId = createAndEnterDebate("The bridge should close");
    const int rootId = currentClaimId();

    auto sourced = baseEvent(debate_event::ADD_CHILD_CLAIM);
    sourced.mutable_add_child_claim()->set_claim("The bridge is unsafe");
    sourced.mutable_add_child_claim()->set_voice("the reporter");
    sourced.mutable_add_child_claim()->set_source_url("https://example.org/bridge");
    sourced.mutable_add_child_claim()->set_source_excerpt("engineers called the span unsafe");
    send(sourced);

    auto bare = baseEvent(debate_event::ADD_CHILD_CLAIM);
    bare.mutable_add_child_claim()->set_claim("Traffic on it doubled");
    send(bare);

    DebateWrapper& dw = moderator_->getDebateWrapper();
    debate::Claim withSource, without;
    for (const auto& m : readMoves(*db_, debateId)) {
        if (m.type != "ASSERT" || m.targetType != "claim" || m.targetId == rootId) continue;
        const debate::Claim c = dw.getClaimById(m.targetId);
        if (c.sentence() == "The bridge is unsafe") withSource = c; else without = c;
    }
    ASSERT_GT(withSource.id(), 0);
    ASSERT_GT(without.id(), 0);

    ASSERT_EQ(withSource.sources_size(), 1);
    EXPECT_EQ(withSource.sources(0).url(), "https://example.org/bridge");
    EXPECT_EQ(withSource.sources(0).excerpt(), "engineers called the span unsafe");
    EXPECT_EQ(without.sources_size(), 0) << "blank source boxes add nothing";
}

TEST_F(MoveLogTest, AChallengeCarriesItsSource) {
    createAndEnterDebate("The bridge should close");
    const int rootId = currentClaimId();

    auto start = baseEvent(debate_event::START_CHALLENGE_CLAIM);
    send(start);
    auto addTarget = baseEvent(debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
    addTarget.mutable_add_claim_to_be_challenged()->set_claim_id(rootId);
    send(addTarget);
    auto open = baseEvent(debate_event::OPEN_ADD_CHALLENGE);
    send(open);
    auto submit = baseEvent(debate_event::SUBMIT_CHALLENGE_CLAIM);
    submit.mutable_submit_challenge_claim()->set_challenge_sentence("The bridge passed inspection");
    submit.mutable_submit_challenge_claim()->set_source_url("https://example.org/inspection");
    send(submit);

    const int challengeId = currentClaimId();
    ASSERT_NE(challengeId, rootId);
    const debate::Claim challenge = moderator_->getDebateWrapper().getClaimById(challengeId);
    ASSERT_EQ(challenge.sources_size(), 1);
    EXPECT_EQ(challenge.sources(0).url(), "https://example.org/inspection");
    EXPECT_EQ(challenge.sources(0).excerpt(), "") << "a link alone is a source";
}


// ---------------------------------------------------------------------------
// Agreement: "I hold this true" from a reader's own view.
// ---------------------------------------------------------------------------
using AgreementFlowTest = UndercutFlowTest;

TEST_F(AgreementFlowTest, AgreeingHoldsTheClaimTrueForTheReaderAndForNobodyElse) {
    const int debateId = createAndEnterDebate("The bridge should close");
    const int root = currentClaimId();
    const int bob = userNamed("bob");
    const int carol = userNamed("carol");
    joinAndEnter(bob, "bob", debateId);
    joinAndEnter(carol, "carol", debateId);

    EXPECT_TRUE(undetermined(shown(debateId, root, "bob"))) << "no view yet";

    auto agree = eventFor(bob, "bob", debate_event::ENDORSE_CLAIM);
    agree.mutable_endorse_claim()->set_claim_id(root);
    send(agree);

    // Logged, and shown.
    bool logged = false;
    for (const auto& m : readMoves(*db_, debateId)) {
        if (m.type == "ACCEPT" && m.actorId == bob && m.targetType == "claim" && m.targetId == root) logged = true;
    }
    EXPECT_TRUE(logged) << "an ACCEPT by bob against the root";
    EXPECT_EQ(show(shown(debateId, root, "bob")), "TRUE");
    EXPECT_EQ(show(shown(debateId, root, "alice")), "TRUE") << "the author's view is untouched";
    EXPECT_TRUE(undetermined(shown(debateId, root, "carol"))) << "nobody else's either";

    // Taking it back removes the entry, and the replay agrees.
    auto withdraw = eventFor(bob, "bob", debate_event::UNENDORSE_CLAIM);
    withdraw.mutable_unendorse_claim()->set_claim_id(root);
    send(withdraw);

    bool unlogged = false;
    for (const auto& m : readMoves(*db_, debateId)) {
        if (m.type == "UNACCEPT" && m.actorId == bob && m.targetId == root) unlogged = true;
    }
    EXPECT_TRUE(unlogged);
    EXPECT_TRUE(undetermined(shown(debateId, root, "bob")));
    EXPECT_EQ(show(shown(debateId, root, "alice")), "TRUE");
}

// Agreement is a seed, not a verdict: the graph still rules. Bob agrees with
// the root, then holds a challenge against it -- the challenge wins for Bob.
TEST_F(AgreementFlowTest, ALaterChallengeOverridesAnEarlierAgreement) {
    const int debateId = createAndEnterDebate("The bridge should close");
    const int root = currentClaimId();
    const int bob = userNamed("bob");
    joinAndEnter(bob, "bob", debateId);

    auto agree = eventFor(bob, "bob", debate_event::ENDORSE_CLAIM);
    agree.mutable_endorse_claim()->set_claim_id(root);
    send(agree);
    EXPECT_EQ(show(shown(debateId, root, "bob")), "TRUE");

    rebut(bob, "bob", root, "The bridge passed inspection last month");
    EXPECT_EQ(show(shown(debateId, root, "bob")), "FALSE") << "his own challenge, held true, refutes it for him";
    EXPECT_EQ(show(shown(debateId, root, "alice")), "TRUE") << "and not for alice";
}

// Without a claim id, the move lands on the claim the user stands on.
TEST_F(AgreementFlowTest, AgreeingWithNoClaimNamedMeansTheClaimTheUserStandsOn) {
    const int debateId = createAndEnterDebate("The bridge should close");
    const int root = currentClaimId();
    const int child = support(aliceId_, "alice", root, "Engineers found cracks in the span");
    const int bob = userNamed("bob");
    joinAndEnter(bob, "bob", debateId);
    goTo(bob, "bob", child);

    auto agree = eventFor(bob, "bob", debate_event::ENDORSE_CLAIM);
    send(agree);
    EXPECT_EQ(show(shown(debateId, child, "bob")), "TRUE");
    // Rule B: the one reason he holds true carries the root for him.
    EXPECT_EQ(show(shown(debateId, root, "bob")), "TRUE");
}


// ---------------------------------------------------------------------------
// Retraction: a deleted claim is out of the debate, and so is everything
// that was done through it. The row stays; the log says it is gone.
// ---------------------------------------------------------------------------
using RetractFlowTest = UndercutFlowTest;

TEST_F(RetractFlowTest, DeletingAChallengeUnrefutesItsTargetForTheChallenger) {
    const int debateId = createAndEnterDebate("The bridge should close");
    const int root = currentClaimId();
    const int bob = userNamed("bob");
    joinAndEnter(bob, "bob", debateId);

    const int challenge = rebut(bob, "bob", root, "The bridge passed inspection");
    ASSERT_GT(challenge, 0);
    EXPECT_EQ(show(shown(debateId, root, "bob")), "FALSE") << "the challenge refutes it for bob";
    EXPECT_EQ(show(shown(debateId, challenge, "bob")), "TRUE");

    auto del = eventFor(bob, "bob", debate_event::DELETE_CHALLENGE);
    del.mutable_delete_challenge()->set_challenge_id(challenge);
    send(del);

    bool retracted = false;
    for (const auto& m : readMoves(*db_, debateId)) {
        if (m.type == "RETRACT" && m.targetType == "claim" && m.targetId == challenge) retracted = true;
    }
    EXPECT_TRUE(retracted);
    EXPECT_TRUE(undetermined(shown(debateId, root, "bob"))) << "the refutation went with the challenge";
    EXPECT_TRUE(undetermined(shown(debateId, challenge, "bob"))) << "a retracted claim has no status for anyone";
    EXPECT_EQ(show(shown(debateId, root, "alice")), "TRUE");
}

TEST_F(RetractFlowTest, DeletingAReasonTakesItsSupportAway) {
    const int debateId = createAndEnterDebate("The bridge should close");
    const int root = currentClaimId();
    const int child = support(aliceId_, "alice", root, "Engineers found cracks in the span");
    const int bob = userNamed("bob");
    joinAndEnter(bob, "bob", debateId);

    // Bob holds the reason, so the root stands on it for him.
    auto agree = eventFor(bob, "bob", debate_event::ENDORSE_CLAIM);
    agree.mutable_endorse_claim()->set_claim_id(child);
    send(agree);
    EXPECT_EQ(show(shown(debateId, root, "bob")), "TRUE");

    goTo(aliceId_, "alice", root);
    auto del = baseEvent(debate_event::DELETE_CHILD_CLAIM);
    del.mutable_delete_child_claim()->set_claim_id(child);
    send(del);

    EXPECT_TRUE(undetermined(shown(debateId, root, "bob"))) << "no reason left for bob";
    EXPECT_TRUE(undetermined(shown(debateId, child, "bob"))) << "and the reason itself is gone";
    EXPECT_EQ(show(shown(debateId, root, "alice")), "TRUE") << "alice asserted the root herself";
}


// ---------------------------------------------------------------------------
// Backfill: a debate with no move log gets one written from its stored
// statuses, and the replay of that log shows what the stored side showed.
// ---------------------------------------------------------------------------
#include "moveLog/MoveBackfill.h"

using BackfillTest = UndercutFlowTest;

TEST_F(BackfillTest, ADebateWithoutALogGetsOneThatReplaysToItsStoredStatuses) {
    // A real debate, played through the moderator so the stored statuses are
    // what the old engine wrote: a reason, a challenge on it, a concession,
    // and an outsider who agrees with the root.
    const int debateId = createAndEnterDebate("The bridge should close");
    const int root = currentClaimId();
    const int reason = support(aliceId_, "alice", root, "Engineers found cracks in the span");
    const int bob = userNamed("bob");
    const int carol = userNamed("carol");
    joinAndEnter(bob, "bob", debateId);
    joinAndEnter(carol, "carol", debateId);
    const int challenge = rebut(bob, "bob", reason, "Those cracks were cosmetic");
    concede(aliceId_, "alice", linkBetween(challenge, reason));
    auto agree = eventFor(carol, "carol", debate_event::ENDORSE_CLAIM);
    agree.mutable_endorse_claim()->set_claim_id(root);
    send(agree);

    // Plant the statuses on the rows, the way the old engine left them, then
    // throw the real log away: this is now a debate from before the log.
    std::map<int, std::map<std::string, debate::ClaimStatus>> stored;
    for (int id : {root, reason, challenge}) {
        debate::Claim c = dw().getClaimById(id);
        c.mutable_user_statuses()->clear();
        for (const char* user : {"alice", "bob", "carol"}) {
            const auto s = shown(debateId, id, user);
            if (s.has_value()) {
                (*c.mutable_user_statuses())[user] = *s;
                stored[id][user] = *s;
            }
        }
        dw().updateClaimInDB(c);
    }
    ASSERT_EQ(stored[reason]["alice"], debate::ClaimStatus::FALSE_CLAIM) << "conceded";
    ASSERT_EQ(stored[root]["carol"], debate::ClaimStatus::TRUE_CLAIM) << "agreed";
    {
        sqlite3_stmt* stmt = db_->prepare("DELETE FROM MOVES WHERE DEBATE_ID = ?;");
        ASSERT_NE(stmt, nullptr);
        sqlite3_bind_int(stmt, 1, debateId);
        ASSERT_EQ(sqlite3_step(stmt), SQLITE_DONE);
        sqlite3_finalize(stmt);
    }
    ASSERT_TRUE(readMoves(*db_, debateId).empty());

    const MoveBackfill::Report r = MoveBackfill::BackfillDebate(debateId, dw(), *wrapper_);
    EXPECT_EQ(r.debatesTouched, 1);
    EXPECT_GT(r.movesWritten, 0);
    EXPECT_TRUE(r.unrepresentable.empty()) << r.unrepresentable[0];

    // Every backfilled move says so.
    for (const auto& m : readMoves(*db_, debateId)) {
        EXPECT_NE(m.payload.find("\"backfilled\":1"), std::string::npos) << m.type;
    }

    // And the replay of the invented log shows every stored position.
    for (const auto& claimEntry : stored) {
        for (const auto& userEntry : claimEntry.second) {
            if (userEntry.second == debate::ClaimStatus::UNDETERMINED) continue;
            EXPECT_EQ(show(shown(debateId, claimEntry.first, userEntry.first)),
                      userEntry.second == debate::ClaimStatus::TRUE_CLAIM ? "TRUE" : "FALSE")
                << "claim " << claimEntry.first << " for " << userEntry.first;
        }
    }

    // Running it again on a debate that now has a log changes nothing.
    const size_t before = readMoves(*db_, debateId).size();
    const MoveBackfill::Report again = MoveBackfill::BackfillDebate(debateId, dw(), *wrapper_);
    EXPECT_EQ(again.debatesTouched, 0);
    EXPECT_EQ(readMoves(*db_, debateId).size(), before);
}
