// test_debate_simulation.cc — Full debate simulation via DebateModerator event pipeline
//
// Instead of calling handler functions directly, this test forges DebateEvent
// protobufs and sends them through DebateModerator::handleRequest().
// This simulates exactly what the virtual renderer does in production.
//
// Flow:
//   1. User A creates a debate  (CREATE_DEBATE event)
//   2. Users A, B, C enter     (ENTER_DEBATE event)
//   3. A adds a child claim     (OPEN_ADD_CHILD_CLAIM + ADD_CHILD_CLAIM events)
//   4. B challenges the child   (START_CHALLENGE_CLAIM + ADD_CLAIM_TO_BE_CHALLENGED
//                                 + OPEN_ADD_CHALLENGE + SUBMIT_CHALLENGE_CLAIM events)
//   5. Verify final state via DB queries
//
// The database persists after the run for manual inspection.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdio>
#include <iostream>

#include "debate.pb.h"
#include "debate_event.pb.h"
#include "user.pb.h"
#include "user_engagement.pb.h"
#include "moderator_to_vr.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "utils/DebateWrapper.h"
#include "debateModerator/DebateModerator.h"

// -------------------------------------------------------
// Helpers
// -------------------------------------------------------
static std::string statusToString(debate::ClaimStatus s) {
    switch (s) {
        case debate::ClaimStatus::UNDETERMINED: return "UNDETERMINED";
        case debate::ClaimStatus::TRUE_CLAIM:   return "TRUE_CLAIM";
        case debate::ClaimStatus::FALSE_CLAIM:  return "FALSE_CLAIM";
        default: return "UNKNOWN(" + std::to_string(static_cast<int>(s)) + ")";
    }
}

// Forge a DebateEvent with the given type and user_id
static debate_event::DebateEvent makeEvent(int user_id, debate_event::EventType type) {
    debate_event::DebateEvent event;
    event.mutable_user()->set_user_id(user_id);
    event.mutable_user()->set_username("user_" + std::to_string(user_id));
    event.mutable_user()->set_is_logged_in(true);
    event.set_type(type);
    return event;
}

// -------------------------------------------------------
// Fixture
// -------------------------------------------------------
class SimulationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a fixed-name test database so it persists after the run
        db_path_ = "test_simulation.sqlite3";
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());

        // Set DB_PATH so DebateModerator uses our test database
        _putenv_s("DB_PATH", db_path_.c_str());

        // Create the DebateModerator (it will use DB_PATH)
        moderator_ = new DebateModerator();

        // Create 3 users via the moderator's user creation
        userA_id_ = moderator_->createUserIfNotExist("A");
        userB_id_ = moderator_->createUserIfNotExist("B");
        userC_id_ = moderator_->createUserIfNotExist("C");

        std::cout << "\n=== DB initialized: " << db_path_ << " ===" << std::endl;
        std::cout << "Users: A=" << userA_id_ << " B=" << userB_id_ << " C=" << userC_id_ << std::endl;
    }

    void TearDown() override {
        delete moderator_;
        // Unset DB_PATH
        _putenv_s("DB_PATH", "");
        std::cout << "\n=== Database preserved at: " << db_path_ << " ===" << std::endl;
        std::cout << "Inspect: sqlite3 " << db_path_ << std::endl;
    }

    // ---- Event sending helpers ----
    void sendEvent(int user_id, debate_event::EventType type) {
        debate_event::DebateEvent event = makeEvent(user_id, type);
        moderator_->handleRequest(event);
    }

    void sendCreateDebate(int user_id, const std::string& topic) {
        debate_event::DebateEvent event = makeEvent(user_id, debate_event::CREATE_DEBATE);
        event.mutable_create_debate()->set_debate_topic(topic);
        moderator_->handleRequest(event);
    }

    void sendEnterDebate(int user_id, int debate_id) {
        debate_event::DebateEvent event = makeEvent(user_id, debate_event::ENTER_DEBATE);
        event.mutable_enter_debate()->set_debate_id(debate_id);
        moderator_->handleRequest(event);
    }

    void sendGoToClaim(int user_id, int claim_id) {
        debate_event::DebateEvent event = makeEvent(user_id, debate_event::GO_TO_CLAIM);
        event.mutable_go_to_claim()->set_claim_id(claim_id);
        moderator_->handleRequest(event);
    }

    void sendOpenAddChildClaim(int user_id) {
        sendEvent(user_id, debate_event::OPEN_ADD_CHILD_CLAIM);
    }

    void sendAddChildClaim(int user_id, const std::string& text, const std::string& description) {
        debate_event::DebateEvent event = makeEvent(user_id, debate_event::ADD_CHILD_CLAIM);
        event.mutable_add_child_claim()->set_claim(text);
        event.mutable_add_child_claim()->set_description(description);
        moderator_->handleRequest(event);
    }

    void sendSubmitChallenge(int user_id, const std::string& sentence) {
        debate_event::DebateEvent event = makeEvent(user_id, debate_event::SUBMIT_CHALLENGE_CLAIM);
        event.mutable_submit_challenge_claim()->set_challenge_sentence(sentence);
        moderator_->handleRequest(event);
    }

    // Note: START_CHALLENGE_CLAIM, ADD_CLAIM_TO_BE_CHALLENGED, and OPEN_ADD_CHALLENGE
    // are purely UI state (modal flags). They're not needed for the submit to work.

    // ---- DB query helpers (read directly from DB, not through moderator) ----
    debate::Claim getClaim(int claim_id) {
        // Use a fresh DatabaseWrapper to read from the same DB
        Database db(db_path_);
        DatabaseWrapper dbWrapper(db);
        DebateWrapper dw(dbWrapper);
        return dw.getClaimById(claim_id);
    }

    debate::ClaimStatus getUserView(int claim_id, const std::string& username) {
        debate::Claim c = getClaim(claim_id);
        auto it = c.user_statuses().find(username);
        if (it != c.user_statuses().end()) return it->second;
        return debate::ClaimStatus::UNDETERMINED;
    }

    int findChallengeClaimId(int challenged_claim_id) {
        Database db(db_path_);
        DatabaseWrapper dbWrapper(db);
        DebateWrapper dw(dbWrapper);
        auto links = dw.getLinksForDebate(1);
        for (const auto& link : links) {
            int linkType = std::get<5>(link);
            if (linkType == static_cast<int>(debate::LinkType::CHALLENGE)) {
                int toClaim = std::get<2>(link);
                if (toClaim == challenged_claim_id) {
                    return std::get<1>(link);
                }
            }
        }
        return -1;
    }

    // ---- State dumper ----
    void dumpState(const std::string& label) {
        std::cout << "\n--- " << label << " ---" << std::endl;
        Database db(db_path_);
        DatabaseWrapper dbWrapper(db);
        DebateWrapper dw(dbWrapper);

        auto statements = dw.getStatementsForDebate(1);
        std::cout << "Claims:" << std::endl;
        for (const auto& stmtBytes : statements) {
            debate::Claim c;
            if (!c.ParseFromArray(stmtBytes.data(), static_cast<int>(stmtBytes.size()))) continue;
            std::cout << "  #" << c.id() << " \"" << c.sentence() << "\""
                      << " status=" << statusToString(c.status())
                      << " creator=" << c.creator_id()
                      << " views={ ";
            for (const auto& kv : c.user_statuses()) {
                std::cout << kv.first << "=" << statusToString(kv.second) << " ";
            }
            std::cout << "}" << std::endl;
        }

        auto links = dw.getLinksForDebate(1);
        std::cout << "Links:" << std::endl;
        for (const auto& link : links) {
            int linkId = std::get<0>(link);
            int from   = std::get<1>(link);
            int to     = std::get<2>(link);
            int type   = std::get<5>(link);
            std::string typeStr = (type == 0) ? "NORMAL" : (type == 1) ? "PARENT_CHILD" : "CHALLENGE";
            std::cout << "  #" << linkId << " " << from << " -> " << to << " [" << typeStr << "]" << std::endl;
        }
    }

    std::string db_path_;
    DebateModerator* moderator_ = nullptr;
    int userA_id_ = 0;
    int userB_id_ = 0;
    int userC_id_ = 0;
};

// ============================================================
// TEST 1: CreateDebate via event pipeline
// ============================================================
TEST_F(SimulationTest, CreateDebate) {
    sendCreateDebate(userA_id_, "Is AI beneficial?");

    debate::Claim root = getClaim(1);
    ASSERT_EQ(root.id(), 1);
    ASSERT_EQ(root.sentence(), "Is AI beneficial?");

    // A (creator) sees root as TRUE_CLAIM
    EXPECT_EQ(getUserView(1, "A"), debate::ClaimStatus::TRUE_CLAIM);
    // B and C have no entry
    EXPECT_EQ(getUserView(1, "B"), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(getUserView(1, "C"), debate::ClaimStatus::UNDETERMINED);

    dumpState("CreateDebate");
}

// ============================================================
// TEST 2: EnterDebate via event pipeline
// ============================================================
TEST_F(SimulationTest, EnterDebate) {
    sendCreateDebate(userA_id_, "Is AI beneficial?");
    sendEnterDebate(userA_id_, 1);
    sendEnterDebate(userB_id_, 1);
    sendEnterDebate(userC_id_, 1);

    // All users should be in the debate now
    // We verify by checking the moderator's response
    debate_event::DebateEvent event = makeEvent(userA_id_, debate_event::NONE);
    moderator_to_vr::ModeratorToVRMessage response = moderator_->handleRequest(event);

    EXPECT_EQ(response.user().engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(response.user().engagement().debating_info().debate_id(), 1);

    dumpState("EnterDebate");
}

// ============================================================
// TEST 3: AddChildClaim via event pipeline
// ============================================================
TEST_F(SimulationTest, AddChildClaim) {
    sendCreateDebate(userA_id_, "Is AI beneficial?");
    sendEnterDebate(userA_id_, 1);
    sendEnterDebate(userB_id_, 1);
    sendEnterDebate(userC_id_, 1);

    // A adds a child claim
    sendGoToClaim(userA_id_, 1);  // navigate to root
    sendOpenAddChildClaim(userA_id_);
    sendAddChildClaim(userA_id_, "AI improves healthcare", "supports");

    // Find the child claim
    Database db(db_path_);
    DatabaseWrapper dbWrapper(db);
    DebateWrapper dw(dbWrapper);
    std::vector<int> children = dw.findChildrenIds(1);
    ASSERT_EQ(children.size(), 1u) << "Root should have exactly 1 child";
    int childId = children[0];

    debate::Claim child = getClaim(childId);
    EXPECT_EQ(child.sentence(), "AI improves healthcare");

    // A (creator) sees child as TRUE_CLAIM
    EXPECT_EQ(getUserView(childId, "A"), debate::ClaimStatus::TRUE_CLAIM);
    // B and C have no entry
    EXPECT_EQ(getUserView(childId, "B"), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(getUserView(childId, "C"), debate::ClaimStatus::UNDETERMINED);

    dumpState("AddChildClaim");
}

// ============================================================
// TEST 4: ChallengeClaim via event pipeline
// ============================================================
TEST_F(SimulationTest, ChallengeClaim) {
    sendCreateDebate(userA_id_, "Is AI beneficial?");
    sendEnterDebate(userA_id_, 1);
    sendEnterDebate(userB_id_, 1);
    sendEnterDebate(userC_id_, 1);

    // A adds a child claim
    sendGoToClaim(userA_id_, 1);
    sendOpenAddChildClaim(userA_id_);
    sendAddChildClaim(userA_id_, "AI improves healthcare", "supports");

    // Get the child claim ID
    Database db(db_path_);
    DatabaseWrapper dbWrapper(db);
    DebateWrapper dw(dbWrapper);
    std::vector<int> children = dw.findChildrenIds(1);
    ASSERT_EQ(children.size(), 1u);
    int childId = children[0];

    // B challenges the child claim (minimal flow: GO_TO_CLAIM + SUBMIT_CHALLENGE_CLAIM)
    // Note: START_CHALLENGE_CLAIM, ADD_CLAIM_TO_BE_CHALLENGED, OPEN_ADD_CHALLENGE are
    // purely UI state (modal flags). SubmitChallengeClaim only needs current_claim_id
    // from user protobuf, which is set by GO_TO_CLAIM.
    sendGoToClaim(userB_id_, childId);
    sendSubmitChallenge(userB_id_, "Healthcare AI has bias issues");

    // Verify challenge claim was created
    int challengeClaimId = findChallengeClaimId(childId);
    ASSERT_GT(challengeClaimId, 0) << "Challenge claim should exist";

    debate::Claim challengeClaim = getClaim(challengeClaimId);
    EXPECT_EQ(challengeClaim.sentence(), "Healthcare AI has bias issues");

    // Verify per-user views on the challenged claim
    EXPECT_EQ(getUserView(childId, "A"), debate::ClaimStatus::TRUE_CLAIM)
        << "A (creator) should see child as TRUE_CLAIM";
    EXPECT_EQ(getUserView(childId, "B"), debate::ClaimStatus::FALSE_CLAIM)
        << "B (challenger) should see child as FALSE_CLAIM";
    EXPECT_EQ(getUserView(childId, "C"), debate::ClaimStatus::UNDETERMINED)
        << "C (uninvolved) should see child as UNDETERMINED";

    // Verify per-user views on the challenge claim
    EXPECT_EQ(getUserView(challengeClaimId, "B"), debate::ClaimStatus::TRUE_CLAIM)
        << "B (creator of challenge) should see challenge claim as TRUE_CLAIM";
    EXPECT_EQ(getUserView(challengeClaimId, "A"), debate::ClaimStatus::UNDETERMINED)
        << "A (not creator) should see challenge claim as UNDETERMINED";

    dumpState("ChallengeClaim");
}

// ============================================================
// TEST 5: Full state verification after all steps
// ============================================================
TEST_F(SimulationTest, FullStateVerification) {
    // Step 1: Create debate
    sendCreateDebate(userA_id_, "Is AI beneficial?");

    // Step 2: All users enter
    sendEnterDebate(userA_id_, 1);
    sendEnterDebate(userB_id_, 1);
    sendEnterDebate(userC_id_, 1);

    // Step 3: A adds child
    sendGoToClaim(userA_id_, 1);
    sendOpenAddChildClaim(userA_id_);
    sendAddChildClaim(userA_id_, "AI improves healthcare", "supports");

    // Get child ID
    Database db(db_path_);
    DatabaseWrapper dbWrapper(db);
    DebateWrapper dw(dbWrapper);
    std::vector<int> children = dw.findChildrenIds(1);
    ASSERT_EQ(children.size(), 1u);
    int childId = children[0];

    // Step 4: B challenges child (minimal flow)
    sendGoToClaim(userB_id_, childId);
    sendSubmitChallenge(userB_id_, "Healthcare AI has bias issues");

    int challengeClaimId = findChallengeClaimId(childId);
    ASSERT_GT(challengeClaimId, 0);

    // ---- Comprehensive verification ----

    // 1. Root claim exists and is correct
    debate::Claim root = getClaim(1);
    ASSERT_EQ(root.id(), 1);
    ASSERT_EQ(root.sentence(), "Is AI beneficial?");
    ASSERT_TRUE(dw.isRoot(1));

    // 2. Child claim exists and is correct
    debate::Claim child = getClaim(childId);
    ASSERT_EQ(child.sentence(), "AI improves healthcare");

    // 3. Challenge claim exists and is correct
    debate::Claim challenge = getClaim(challengeClaimId);
    ASSERT_EQ(challenge.sentence(), "Healthcare AI has bias issues");

    // 4. Links: parent-child + challenge
    auto links = dw.getLinksForDebate(1);
    int pcCount = 0, chCount = 0;
    for (const auto& link : links) {
        int type = std::get<5>(link);
        if (type == static_cast<int>(debate::LinkType::PARENT_CHILD)) pcCount++;
        if (type == static_cast<int>(debate::LinkType::CHALLENGE)) chCount++;
    }
    EXPECT_EQ(pcCount, 1) << "Should have 1 parent-child link";
    EXPECT_EQ(chCount, 1) << "Should have 1 challenge link";

    // 5. Per-user views on root
    EXPECT_EQ(getUserView(1, "A"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(getUserView(1, "B"), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(getUserView(1, "C"), debate::ClaimStatus::UNDETERMINED);

    // 6. Per-user views on child
    EXPECT_EQ(getUserView(childId, "A"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(getUserView(childId, "B"), debate::ClaimStatus::FALSE_CLAIM);
    EXPECT_EQ(getUserView(childId, "C"), debate::ClaimStatus::UNDETERMINED);

    // 7. Per-user views on challenge claim
    EXPECT_EQ(getUserView(challengeClaimId, "B"), debate::ClaimStatus::TRUE_CLAIM);
    EXPECT_EQ(getUserView(challengeClaimId, "A"), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(getUserView(challengeClaimId, "C"), debate::ClaimStatus::UNDETERMINED);

    // 8. User engagement: all users in debate
    debate_event::DebateEvent eventA = makeEvent(userA_id_, debate_event::NONE);
    auto respA = moderator_->handleRequest(eventA);
    EXPECT_EQ(respA.user().engagement().current_action(), user_engagement::ACTION_DEBATING);

    debate_event::DebateEvent eventB = makeEvent(userB_id_, debate_event::NONE);
    auto respB = moderator_->handleRequest(eventB);
    EXPECT_EQ(respB.user().engagement().current_action(), user_engagement::ACTION_DEBATING);

    debate_event::DebateEvent eventC = makeEvent(userC_id_, debate_event::NONE);
    auto respC = moderator_->handleRequest(eventC);
    EXPECT_EQ(respC.user().engagement().current_action(), user_engagement::ACTION_DEBATING);

    dumpState("FullStateVerification");
}
