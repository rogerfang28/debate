// test_debate_simulation.cc — Full debate simulation test
//
// Simulates a complete debate scenario with 3 users (A, B, C):
//   1. User A creates a debate
//   2. Users B and C join
//   3. User A adds a child claim under root
//   4. User B challenges that child claim
//
// Each step checks the database state via assertions.
// The test database is preserved after the run for manual inspection.
//
// Expandable: add more steps after the existing ones.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdio>
#include <iostream>

#include "debate.pb.h"
#include "user.pb.h"
#include "user_engagement.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "utils/DebateWrapper.h"

#include "debateModerator/event-handlers/DebateHandler/DebateHandler.h"
#include "debateModerator/event-handlers/MoveUserHandler/MoveUserHandler.h"
#include "debateModerator/event-handlers/AddClaimHandler/AddClaimHandler.h"
#include "debateModerator/event-handlers/ChallengeHandler/ChallengeHandler.h"
#include "debateModerator/event-handlers/DeleteClaimHandler/DeleteClaimHandler.h"

// -------------------------------------------------------
// Helpers
// -------------------------------------------------------
static user::User makeUser(int userId, const std::string& name,
                           user_engagement::EngagementAction action) {
    user::User u;
    u.set_user_id(userId);
    u.set_username(name);
    u.mutable_engagement()->set_current_action(action);
    return u;
}

static std::vector<uint8_t> serializeUser(const user::User& u) {
    std::vector<uint8_t> data(u.ByteSizeLong());
    u.SerializeToArray(data.data(), data.size());
    return data;
}

static std::string statusToString(debate::ClaimStatus s) {
    switch (s) {
        case debate::ClaimStatus::UNDETERMINED: return "UNDETERMINED";
        case debate::ClaimStatus::TRUE_CLAIM:   return "TRUE_CLAIM";
        case debate::ClaimStatus::FALSE_CLAIM:  return "FALSE_CLAIM";
        default: return "UNKNOWN(" + std::to_string(static_cast<int>(s)) + ")";
    }
}

// -------------------------------------------------------
// Fixture — persistent database for post-mortem debugging
// -------------------------------------------------------
class DebateSimulationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a fixed-name database so it persists after the test
        db_path_ = "test_simulation.sqlite3";
        // Always start fresh — delete any leftover from a previous run
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());

        db_      = new Database(db_path_);
        wrapper_ = new DatabaseWrapper(*db_);
        debate_  = new DebateWrapper(*wrapper_);
        ASSERT_TRUE(wrapper_->ensureAllTables());

        // Create User A
        userA_ = makeUser(0, "A", user_engagement::ACTION_HOME);
        userA_id_ = wrapper_->users.createUser("A", serializeUser(userA_));
        ASSERT_GT(userA_id_, 0);
        userA_.set_user_id(userA_id_);
        wrapper_->users.updateUserProtobuf(userA_id_, serializeUser(userA_));

        // Create User B
        userB_ = makeUser(0, "B", user_engagement::ACTION_HOME);
        userB_id_ = wrapper_->users.createUser("B", serializeUser(userB_));
        ASSERT_GT(userB_id_, 0);
        userB_.set_user_id(userB_id_);
        wrapper_->users.updateUserProtobuf(userB_id_, serializeUser(userB_));

        // Create User C
        userC_ = makeUser(0, "C", user_engagement::ACTION_HOME);
        userC_id_ = wrapper_->users.createUser("C", serializeUser(userC_));
        ASSERT_GT(userC_id_, 0);
        userC_.set_user_id(userC_id_);
        wrapper_->users.updateUserProtobuf(userC_id_, serializeUser(userC_));
    }

    void TearDown() override {
        delete debate_;
        delete wrapper_;
        delete db_;
        // NOTE: We do NOT delete db_path_ — the database is kept for inspection.
        // To clean up, manually delete test_simulation.sqlite3
        std::cout << "\n=== Database preserved at: " << db_path_ << " ===" << std::endl;
    }

    // ---- User helpers ----
    void actAsA() { userId_ = userA_id_; user_ = &userA_; }
    void actAsB() { userId_ = userB_id_; user_ = &userB_; }
    void actAsC() { userId_ = userC_id_; user_ = &userC_; }

    void refreshUserA() { userA_ = debate_->getUserProtobuf(userA_id_); }
    void refreshUserB() { userB_ = debate_->getUserProtobuf(userB_id_); }
    void refreshUserC() { userC_ = debate_->getUserProtobuf(userC_id_); }

    void enterDebate(int debateId) {
        MoveUserHandler::EnterDebate(debateId, userId_, *debate_);
        refreshCurrentUser();
    }

    void addChild(int parentId, const std::string& text,
                  const std::string& connection) {
        AddClaimHandler::OpenAddChildClaim(userId_, *debate_);
        AddClaimHandler::AddClaimUnderClaim(text, connection, userId_, *debate_);
        refreshCurrentUser();
    }

    void goToClaim(int claimId) {
        MoveUserHandler::GoToClaim(claimId, userId_, *debate_);
        refreshCurrentUser();
    }

    void challengeCurrentClaim(const std::string& reason) {
        ChallengeHandler::StartChallengeClaim(userId_, *debate_);
        ChallengeHandler::AddClaimToBeChallenged(
            user_->engagement().debating_info().current_claim().id(),
            userId_, *debate_);
        ChallengeHandler::OpenAddChallenge(userId_, *debate_);
        ChallengeHandler::SubmitChallengeClaim(reason, userId_, *debate_);
        refreshCurrentUser();
    }

    void refreshCurrentUser() {
        if (userId_ == userA_id_) refreshUserA();
        else if (userId_ == userB_id_) refreshUserB();
        else if (userId_ == userC_id_) refreshUserC();
    }

    // ---- Claim helpers ----
    debate::Claim getClaim(int id) { return debate_->getClaimById(id); }

    // Get a user's view of a claim's status from the user_statuses map
    debate::ClaimStatus getUserView(const debate::Claim& claim, const std::string& username) {
        auto it = claim.user_statuses().find(username);
        if (it != claim.user_statuses().end()) {
            return it->second;
        }
        return debate::ClaimStatus::UNDETERMINED;  // default if not in map
    }

    // Find the challenge claim targeting a given claim
    int findChallengeClaimId(int challengedClaimId) {
        auto links = debate_->getLinksForDebate(1);
        for (const auto& link : links) {
            int linkType = std::get<5>(link);
            if (linkType == static_cast<int>(debate::LinkType::CHALLENGE)) {
                int toClaim = std::get<2>(link);
                if (toClaim == challengedClaimId) {
                    return std::get<1>(link);  // from = challenge claim
                }
            }
        }
        return -1;
    }

    // ---- State dumper for debugging ----
    void dumpState(const std::string& label) {
        std::cout << "\n===== STATE: " << label << " =====" << std::endl;

        // Dump all claims
        auto statements = debate_->getStatementsForDebate(1);
        std::cout << "\n--- Claims ---" << std::endl;
        for (const auto& stmtBytes : statements) {
            debate::Claim c;
            if (!c.ParseFromArray(stmtBytes.data(), static_cast<int>(stmtBytes.size()))) {
                std::cout << "  [failed to parse claim]" << std::endl;
                continue;
            }
            std::cout << "  Claim " << c.id() << ": \"" << c.sentence() << "\""
                      << " | status=" << statusToString(c.status())
                      << " | creator=" << c.creator_id()
                      << std::endl;
            std::cout << "    user_statuses: { ";
            for (const auto& kv : c.user_statuses()) {
                std::cout << kv.first << "=" << statusToString(kv.second) << " ";
            }
            std::cout << "}" << std::endl;
        }

        // Dump all links
        auto links = debate_->getLinksForDebate(1);
        std::cout << "\n--- Links ---" << std::endl;
        for (const auto& link : links) {
            int linkId = std::get<0>(link);
            int from   = std::get<1>(link);
            int to     = std::get<2>(link);
            int type   = std::get<5>(link);
            std::string typeStr = (type == 0) ? "NORMAL" : (type == 1) ? "PARENT_CHILD" : "CHALLENGE";
            std::cout << "  Link " << linkId << ": " << from << " -> " << to
                      << " [" << typeStr << "]" << std::endl;
        }

        // Dump user engagement
        std::cout << "\n--- User Engagement ---" << std::endl;
        refreshUserA(); refreshUserB(); refreshUserC();
        for (auto* u : {&userA_, &userB_, &userC_}) {
            std::cout << "  " << u->username() << ": action="
                      << u->engagement().current_action()
                      << " debate=" << u->engagement().debating_info().debate_id()
                      << " current_claim=" << u->engagement().debating_info().current_claim().id()
                      << std::endl;
        }
        std::cout << "===== END STATE =====\n" << std::endl;
    }

    std::string db_path_;
    Database*      db_      = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
    DebateWrapper* debate_  = nullptr;

    int userA_id_ = 0;
    int userB_id_ = 0;
    int userC_id_ = 0;
    user::User userA_;
    user::User userB_;
    user::User userC_;

    int userId_ = 0;
    user::User* user_ = nullptr;
};

// ============================================================
// FULL DEBATE SIMULATION
// ============================================================
TEST_F(DebateSimulationTest, FullDebateSimulation) {
    // -------------------------------------------------------
    // STEP 1: User A creates a debate
    // -------------------------------------------------------
    std::cout << "\n>>> STEP 1: User A creates debate" << std::endl;
    actAsA();
    DebateHandler::AddDebate("Is AI beneficial?", userId_, *debate_);
    int debateId = 1;

    // Verify: debate exists with root claim
    debate::Claim root = getClaim(1);
    ASSERT_EQ(root.id(), 1);
    ASSERT_EQ(root.sentence(), "Is AI beneficial?");
    ASSERT_TRUE(debate_->isRoot(1));

    // Root claim: A sees it as TRUE_CLAIM (creator), B and C have no entry
    EXPECT_EQ(getUserView(root, "A"), debate::ClaimStatus::TRUE_CLAIM)
        << "Creator A should see root as TRUE_CLAIM";
    EXPECT_EQ(getUserView(root, "B"), debate::ClaimStatus::UNDETERMINED)
        << "B should see root as UNDETERMINED (not in user_statuses map)";
    EXPECT_EQ(getUserView(root, "C"), debate::ClaimStatus::UNDETERMINED)
        << "C should see root as UNDETERMINED (not in user_statuses map)";

    dumpState("After Step 1: A creates debate");

    // -------------------------------------------------------
    // STEP 2: Users B and C join the debate
    // -------------------------------------------------------
    std::cout << "\n>>> STEP 2: B and C join the debate" << std::endl;

    // A enters first (creator already has debate context)
    actAsA();
    enterDebate(debateId);
    EXPECT_EQ(userA_.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(userA_.engagement().debating_info().debate_id(), debateId);

    // B enters
    actAsB();
    enterDebate(debateId);
    EXPECT_EQ(userB_.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(userB_.engagement().debating_info().debate_id(), debateId);
    EXPECT_EQ(userB_.engagement().debating_info().current_claim().id(), 1);  // at root

    // C enters
    actAsC();
    enterDebate(debateId);
    EXPECT_EQ(userC_.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(userC_.engagement().debating_info().debate_id(), debateId);
    EXPECT_EQ(userC_.engagement().debating_info().current_claim().id(), 1);  // at root

    // All users should be at root claim
    refreshUserA(); refreshUserB(); refreshUserC();
    EXPECT_EQ(userA_.engagement().debating_info().current_claim().id(), 1);
    EXPECT_EQ(userB_.engagement().debating_info().current_claim().id(), 1);
    EXPECT_EQ(userC_.engagement().debating_info().current_claim().id(), 1);

    dumpState("After Step 2: B and C join");

    // -------------------------------------------------------
    // STEP 3: User A adds a child claim under root
    // -------------------------------------------------------
    std::cout << "\n>>> STEP 3: A adds child claim" << std::endl;
    actAsA();
    goToClaim(1);  // ensure A is at root
    addChild(1, "AI improves healthcare", "supports");

    // Find the new child claim
    std::vector<int> rootChildren = debate_->findChildrenIds(1);
    ASSERT_EQ(rootChildren.size(), 1u) << "Root should have exactly 1 child";
    int childId = rootChildren[0];

    debate::Claim child = getClaim(childId);
    EXPECT_EQ(child.sentence(), "AI improves healthcare");

    // Verify parent-child link exists
    auto links = debate_->getLinksForDebate(1);
    bool foundParentChild = false;
    for (const auto& link : links) {
        int type = std::get<5>(link);
        if (type == static_cast<int>(debate::LinkType::PARENT_CHILD)) {
            int from = std::get<1>(link);
            int to   = std::get<2>(link);
            if (from == 1 && to == childId) {
                foundParentChild = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundParentChild) << "Parent-child link should exist between root and child";

    // Verify user_statuses: A (creator) sees child as TRUE_CLAIM
    child = getClaim(childId);  // refresh
    EXPECT_EQ(getUserView(child, "A"), debate::ClaimStatus::TRUE_CLAIM)
        << "A (creator) should see child claim as TRUE_CLAIM";
    EXPECT_EQ(getUserView(child, "B"), debate::ClaimStatus::UNDETERMINED)
        << "B should see child as UNDETERMINED (not creator, not in map)";
    EXPECT_EQ(getUserView(child, "C"), debate::ClaimStatus::UNDETERMINED)
        << "C should see child as UNDETERMINED (not creator, not in map)";

    dumpState("After Step 3: A adds child claim");

    // -------------------------------------------------------
    // STEP 4: User B challenges the child claim
    // -------------------------------------------------------
    std::cout << "\n>>> STEP 4: B challenges child claim" << std::endl;
    actAsB();
    goToClaim(childId);
    challengeCurrentClaim("Healthcare AI has bias issues");

    // Verify: a challenge claim was created
    int challengeClaimId = findChallengeClaimId(childId);
    ASSERT_GT(challengeClaimId, 0) << "A challenge claim should have been created";
    ASSERT_NE(challengeClaimId, childId) << "Challenge claim should be different from challenged claim";

    debate::Claim challengeClaim = getClaim(challengeClaimId);
    EXPECT_EQ(challengeClaim.sentence(), "Healthcare AI has bias issues");

    // Verify: CHALLENGE link exists from challenge claim to challenged claim
    links = debate_->getLinksForDebate(1);
    bool foundChallengeLink = false;
    for (const auto& link : links) {
        int type = std::get<5>(link);
        if (type == static_cast<int>(debate::LinkType::CHALLENGE)) {
            int from = std::get<1>(link);
            int to   = std::get<2>(link);
            if (from == challengeClaimId && to == childId) {
                foundChallengeLink = true;
                break;
            }
        }
    }
    EXPECT_TRUE(foundChallengeLink)
        << "CHALLENGE link should exist from challenge claim to challenged claim";

    // Verify: B sees the challenged child as FALSE_CLAIM (challenger's view)
    child = getClaim(childId);
    EXPECT_EQ(getUserView(child, "B"), debate::ClaimStatus::FALSE_CLAIM)
        << "B (challenger) should see challenged claim as FALSE_CLAIM";

    // Verify: A still sees child as TRUE_CLAIM (creator's view unchanged)
    EXPECT_EQ(getUserView(child, "A"), debate::ClaimStatus::TRUE_CLAIM)
        << "A (creator) should still see child as TRUE_CLAIM";

    // Verify: C sees child as UNDETERMINED (not involved)
    EXPECT_EQ(getUserView(child, "C"), debate::ClaimStatus::UNDETERMINED)
        << "C (uninvolved) should see child as UNDETERMINED";

    // Verify: B sees the challenge claim as TRUE_CLAIM (creator of challenge)
    challengeClaim = getClaim(challengeClaimId);
    EXPECT_EQ(getUserView(challengeClaim, "B"), debate::ClaimStatus::TRUE_CLAIM)
        << "B (creator of challenge claim) should see it as TRUE_CLAIM";

    // Verify: A sees challenge claim as UNDETERMINED (not creator)
    EXPECT_EQ(getUserView(challengeClaim, "A"), debate::ClaimStatus::UNDETERMINED)
        << "A (not creator of challenge) should see challenge claim as UNDETERMINED";

    dumpState("After Step 4: B challenges child claim");

    // -------------------------------------------------------
    // SUMMARY
    // -------------------------------------------------------
    std::cout << "\n========================================" << std::endl;
    std::cout << "  DEBATE SIMULATION COMPLETE" << std::endl;
    std::cout << "  Database: " << db_path_ << std::endl;
    std::cout << "  Inspect with: sqlite3 " << db_path_ << std::endl;
    std::cout << "========================================\n" << std::endl;
}
