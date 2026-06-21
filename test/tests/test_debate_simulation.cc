// test_debate_simulation.cc — Step-by-step debate simulation
//
// Each TEST_F is one check. Run individually or all at once:
//   ./debate_simulation.exe                    # all checks
//   ./debate_simulation.exe --gtest_filter="*CreateDebate*"
//   ./debate_simulation.exe --gtest_filter="*EnterDebate*"
//   ./debate_simulation.exe --gtest_filter="*AddChild*"
//   ./debate_simulation.exe --gtest_filter="*Challenge*"
//
// Database persists after all tests finish (test_simulation.sqlite3).

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

// =====================================================================
// HELPERS
// =====================================================================

static std::string statusToString(debate::ClaimStatus s) {
    switch (s) {
        case debate::ClaimStatus::UNDETERMINED: return "UNDETERMINED";
        case debate::ClaimStatus::TRUE_CLAIM:   return "TRUE_CLAIM";
        case debate::ClaimStatus::FALSE_CLAIM:  return "FALSE_CLAIM";
        default: return "UNKNOWN(" + std::to_string(static_cast<int>(s)) + ")";
    }
}

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

// =====================================================================
// FIXTURE — shared database across all tests
// =====================================================================

class Simulation : public ::testing::Test {
protected:
    // ---- one-time setup/teardown across all TEST_Fs ----
    static void SetUpTestSuite() {
        db_path_ = "test_simulation.sqlite3";
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());

        db_      = new Database(db_path_);
        wrapper_ = new DatabaseWrapper(*db_);
        debate_  = new DebateWrapper(*wrapper_);
        bool ok = wrapper_->ensureAllTables();
        if (!ok) {
            std::cerr << "FATAL: ensureAllTables failed" << std::endl;
            abort();
        }

        // Create 3 users
        userA_ = makeUser(0, "A", user_engagement::ACTION_HOME);
        userA_id_ = wrapper_->users.createUser("A", serializeUser(userA_));
        userA_.set_user_id(userA_id_);
        wrapper_->users.updateUserProtobuf(userA_id_, serializeUser(userA_));

        userB_ = makeUser(0, "B", user_engagement::ACTION_HOME);
        userB_id_ = wrapper_->users.createUser("B", serializeUser(userB_));
        userB_.set_user_id(userB_id_);
        wrapper_->users.updateUserProtobuf(userB_id_, serializeUser(userB_));

        userC_ = makeUser(0, "C", user_engagement::ACTION_HOME);
        userC_id_ = wrapper_->users.createUser("C", serializeUser(userC_));
        userC_.set_user_id(userC_id_);
        wrapper_->users.updateUserProtobuf(userC_id_, serializeUser(userC_));

        std::cout << "\n=== DB initialized: " << db_path_ << " ===" << std::endl;
        std::cout << "Users: A=" << userA_id_ << " B=" << userB_id_ << " C=" << userC_id_ << "\n" << std::endl;
    }

    static void TearDownTestSuite() {
        delete debate_;
        delete wrapper_;
        delete db_;
        std::cout << "\n=== Database preserved at: " << db_path_ << " ===" << std::endl;
        std::cout << "Inspect: sqlite3 " << db_path_ << "\n" << std::endl;
    }

    // ---- per-test helpers ----
    void actAs(int userId) { userId_ = userId; }

    void refreshAllUsers() {
        userA_ = debate_->getUserProtobuf(userA_id_);
        userB_ = debate_->getUserProtobuf(userB_id_);
        userC_ = debate_->getUserProtobuf(userC_id_);
    }

    // ---- action helpers (each calls the full event pipeline) ----
    void createDebate(const std::string& topic, int userId) {
        actAs(userId);
        DebateHandler::AddDebate(topic, userId, *debate_);
    }

    void enterDebate(int debateId, int userId) {
        actAs(userId);
        MoveUserHandler::EnterDebate(debateId, userId, *debate_);
    }

    void goToClaim(int claimId, int userId) {
        actAs(userId);
        MoveUserHandler::GoToClaim(claimId, userId, *debate_);
    }

    void addChild(int parentId, const std::string& text,
                  const std::string& connection, int userId) {
        actAs(userId);
        AddClaimHandler::OpenAddChildClaim(userId, *debate_);
        AddClaimHandler::AddClaimUnderClaim(text, connection, userId, *debate_);
    }

    void challengeClaim(int claimId, const std::string& reason, int userId) {
        actAs(userId);
        // Navigate to the claim first
        MoveUserHandler::GoToClaim(claimId, userId, *debate_);
        // Full challenge pipeline
        ChallengeHandler::StartChallengeClaim(userId, *debate_);
        ChallengeHandler::AddClaimToBeChallenged(claimId, userId, *debate_);
        ChallengeHandler::OpenAddChallenge(userId, *debate_);
        ChallengeHandler::SubmitChallengeClaim(reason, userId, *debate_);
    }

    // ---- query helpers ----
    debate::Claim getClaim(int id) { return debate_->getClaimById(id); }

    debate::ClaimStatus userView(int claimId, const std::string& username) {
        debate::Claim c = getClaim(claimId);
        auto it = c.user_statuses().find(username);
        if (it != c.user_statuses().end()) return it->second;
        return debate::ClaimStatus::UNDETERMINED;
    }

    int findChallengeClaim(int challengedClaimId) {
        auto links = debate_->getLinksForDebate(1);
        for (const auto& link : links) {
            if (std::get<5>(link) == static_cast<int>(debate::LinkType::CHALLENGE) &&
                std::get<2>(link) == challengedClaimId) {
                return std::get<1>(link);
            }
        }
        return -1;
    }

    bool hasLink(int from, int to, debate::LinkType type) {
        auto links = debate_->getLinksForDebate(1);
        for (const auto& link : links) {
            if (std::get<1>(link) == from &&
                std::get<2>(link) == to &&
                std::get<5>(link) == static_cast<int>(type)) {
                return true;
            }
        }
        return false;
    }

    // ---- debug printer ----
    void printClaims() {
        auto stmts = debate_->getStatementsForDebate(1);
        std::cout << "  Claims:" << std::endl;
        for (const auto& bytes : stmts) {
            debate::Claim c;
            c.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()));
            std::cout << "    #" << c.id() << " \"" << c.sentence() << "\""
                      << " status=" << statusToString(c.status())
                      << " creator=" << c.creator_id()
                      << " views={";
            for (const auto& kv : c.user_statuses())
                std::cout << " " << kv.first << "=" << statusToString(kv.second);
            std::cout << " }" << std::endl;
        }
    }

    void printLinks() {
        auto links = debate_->getLinksForDebate(1);
        std::cout << "  Links:" << std::endl;
        for (const auto& link : links) {
            std::string typeStr;
            switch (std::get<5>(link)) {
                case 0: typeStr = "NORMAL"; break;
                case 1: typeStr = "PARENT_CHILD"; break;
                case 2: typeStr = "CHALLENGE"; break;
                default: typeStr = "?"; break;
            }
            std::cout << "    #" << std::get<0>(link)
                      << " " << std::get<1>(link) << " -> " << std::get<2>(link)
                      << " [" << typeStr << "]" << std::endl;
        }
    }

    void printUsers() {
        refreshAllUsers();
        std::cout << "  Users:" << std::endl;
        for (auto* u : {&userA_, &userB_, &userC_}) {
            std::cout << "    " << u->username()
                      << " action=" << u->engagement().current_action()
                      << " debate=" << u->engagement().debating_info().debate_id()
                      << " at_claim=" << u->engagement().debating_info().current_claim().id()
                      << std::endl;
        }
    }

    // ---- state ----
    static std::string db_path_;
    static Database* db_;
    static DatabaseWrapper* wrapper_;
    static DebateWrapper* debate_;
    static int userA_id_, userB_id_, userC_id_;
    static user::User userA_, userB_, userC_;
    int userId_ = 0;
    int childId_ = 0;  // set by AddChildClaim, used by ChallengeClaim
};

// static member definitions
std::string Simulation::db_path_;
Database* Simulation::db_ = nullptr;
DatabaseWrapper* Simulation::wrapper_ = nullptr;
DebateWrapper* Simulation::debate_ = nullptr;
int Simulation::userA_id_ = 0;
int Simulation::userB_id_ = 0;
int Simulation::userC_id_ = 0;
user::User Simulation::userA_;
user::User Simulation::userB_;
user::User Simulation::userC_;

// =====================================================================
// CHECK 1: Create debate
// =====================================================================
TEST_F(Simulation, CreateDebate) {
    std::cout << "\n--- CHECK: CreateDebate ---" << std::endl;

    createDebate("Is AI beneficial?", userA_id_);

    debate::Claim root = getClaim(1);
    ASSERT_EQ(root.id(), 1);
    ASSERT_EQ(root.sentence(), "Is AI beneficial?");
    ASSERT_TRUE(debate_->isRoot(1));

    EXPECT_EQ(userView(1, "A"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(userView(1, "B"), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(userView(1, "C"), debate::ClaimStatus::UNDETERMINED);

    printClaims();
    std::cout << "  PASS" << std::endl;
}

// =====================================================================
// CHECK 2: Enter debate
// =====================================================================
TEST_F(Simulation, EnterDebate) {
    std::cout << "\n--- CHECK: EnterDebate ---" << std::endl;

    enterDebate(1, userA_id_);
    enterDebate(1, userB_id_);
    enterDebate(1, userC_id_);

    refreshAllUsers();

    // All users should be in the debate
    EXPECT_EQ(userA_.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(userA_.engagement().debating_info().debate_id(), 1);
    EXPECT_EQ(userA_.engagement().debating_info().current_claim().id(), 1);

    EXPECT_EQ(userB_.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(userB_.engagement().debating_info().debate_id(), 1);
    EXPECT_EQ(userB_.engagement().debating_info().current_claim().id(), 1);

    EXPECT_EQ(userC_.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(userC_.engagement().debating_info().debate_id(), 1);
    EXPECT_EQ(userC_.engagement().debating_info().current_claim().id(), 1);

    printUsers();
    std::cout << "  PASS" << std::endl;
}

// =====================================================================
// CHECK 3: Add child claim
// =====================================================================
TEST_F(Simulation, AddChildClaim) {
    std::cout << "\n--- CHECK: AddChildClaim ---" << std::endl;

    goToClaim(1, userA_id_);
    addChild(1, "AI improves healthcare", "supports", userA_id_);

    // Find the child
    std::vector<int> children = debate_->findChildrenIds(1);
    ASSERT_EQ(children.size(), 1u);
    childId_ = children[0];

    debate::Claim child = getClaim(childId_);
    EXPECT_EQ(child.sentence(), "AI improves healthcare");

    // Parent-child link should exist
    EXPECT_TRUE(hasLink(1, childId_, debate::LinkType::PARENT_CHILD));

    // Per-user views
    EXPECT_EQ(userView(childId_, "A"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(userView(childId_, "B"), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(userView(childId_, "C"), debate::ClaimStatus::UNDETERMINED);

    printClaims();
    printLinks();
    std::cout << "  PASS" << std::endl;
}

// =====================================================================
// CHECK 4: Challenge a claim
// =====================================================================
TEST_F(Simulation, ChallengeClaim) {
    std::cout << "\n--- CHECK: ChallengeClaim ---" << std::endl;

    // childId_ was set by the previous test (AddChildClaim)
    // If running standalone, add the child first:
    if (childId_ == 0) {
        goToClaim(1, userA_id_);
        addChild(1, "AI improves healthcare", "supports", userA_id_);
        childId_ = debate_->findChildrenIds(1)[0];
    }

    challengeClaim(childId_, "Healthcare AI has bias issues", userB_id_);

    // Challenge claim should exist
    int challengeId = findChallengeClaim(childId_);
    ASSERT_GT(challengeId, 0);
    EXPECT_NE(challengeId, childId_);

    debate::Claim challengeClaim = getClaim(challengeId);
    EXPECT_EQ(challengeClaim.sentence(), "Healthcare AI has bias issues");

    // CHALLENGE link should exist
    EXPECT_TRUE(hasLink(challengeId, childId_, debate::LinkType::CHALLENGE));

    // Per-user views on the challenged claim
    EXPECT_EQ(userView(childId_, "A"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(userView(childId_, "B"), debate::ClaimStatus::FALSE_CLAIM);
    EXPECT_EQ(userView(childId_, "C"), debate::ClaimStatus::UNDETERMINED);

    // Per-user views on the challenge claim
    EXPECT_EQ(userView(challengeId, "B"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(userView(challengeId, "A"), debate::ClaimStatus::UNDETERMINED);

    printClaims();
    printLinks();
    std::cout << "  PASS" << std::endl;
}

// =====================================================================
// CHECK 5: Verify full state after all steps
// =====================================================================
TEST_F(Simulation, FullStateVerification) {
    std::cout << "\n--- CHECK: FullStateVerification ---" << std::endl;

    // Ensure childId_ is set (depends on test order)
    if (childId_ == 0) {
        goToClaim(1, userA_id_);
        addChild(1, "AI improves healthcare", "supports", userA_id_);
        childId_ = debate_->findChildrenIds(1)[0];
    }

    int challengeId = findChallengeClaim(childId_);
    if (challengeId == -1) {
        challengeClaim(childId_, "Healthcare AI has bias issues", userB_id_);
        challengeId = findChallengeClaim(childId_);
    }

    // ---- Verify all claims ----
    debate::Claim root = getClaim(1);
    debate::Claim child = getClaim(childId_);
    debate::Claim challenge = getClaim(challengeId);

    EXPECT_EQ(root.sentence(), "Is AI beneficial?");
    EXPECT_EQ(child.sentence(), "AI improves healthcare");
    EXPECT_EQ(challenge.sentence(), "Healthcare AI has bias issues");

    // ---- Verify all links ----
    EXPECT_TRUE(hasLink(1, childId_, debate::LinkType::PARENT_CHILD));
    EXPECT_TRUE(hasLink(challengeId, childId_, debate::LinkType::CHALLENGE));

    // ---- Verify per-user views on root ----
    EXPECT_EQ(userView(1, "A"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(userView(1, "B"), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(userView(1, "C"), debate::ClaimStatus::UNDETERMINED);

    // ---- Verify per-user views on child ----
    EXPECT_EQ(userView(childId_, "A"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(userView(childId_, "B"), debate::ClaimStatus::FALSE_CLAIM);
    EXPECT_EQ(userView(childId_, "C"), debate::ClaimStatus::UNDETERMINED);

    // ---- Verify per-user views on challenge ----
    EXPECT_EQ(userView(challengeId, "A"), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(userView(challengeId, "B"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(userView(challengeId, "C"), debate::ClaimStatus::UNDETERMINED);

    // ---- Verify user engagement ----
    refreshAllUsers();
    EXPECT_EQ(userA_.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(userB_.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(userC_.engagement().current_action(), user_engagement::ACTION_DEBATING);

    printClaims();
    printLinks();
    printUsers();
    std::cout << "  PASS" << std::endl;
}
