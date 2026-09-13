// test_challenge_propagation.cc — Challenge & status propagation tests
//
// Tests the full challenge lifecycle and status propagation through the
// debate graph, using the event handler pipeline:
//
//   1. Build a debate tree (via DebateHandler, AddClaimHandler, etc.)
//   2. Issue a challenge (via ChallengeHandler::SubmitChallengeClaim)
//   3. Verify status propagation through the graph
//
// On the refactor branch, ClaimStatus is: UNDETERMINED, TRUE_CLAIM, FALSE_CLAIM.
// There is no CHALLENGED status. Challenge resolution is determined by
// the challenge claim's own status (TRUE_CLAIM = defended, FALSE_CLAIM = disproven).

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>

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
// Helper: Build a fresh user engagement protobuf
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

// -------------------------------------------------------
// Fixture: sets up DB + 2 users (Alice & Bob)
// -------------------------------------------------------
class PropagationTest : public ::testing::Test {
protected:
    void SetUp() override {
         // Use unique temp file per test process to avoid race conditions in parallel runs
         db_path_ = std::string("test_prop_temp_") + std::to_string(getpid()) + ".sqlite3";
         std::remove(db_path_.c_str());

        db_      = new Database(db_path_);
        // Debates and users share one temp file in tests; production splits them
        // across debates.sqlite3 and users.sqlite3.
        wrapper_ = new DatabaseWrapper(*db_, *db_);
        debate_  = new DebateWrapper(*wrapper_);
        ASSERT_TRUE(wrapper_->ensureAllTables());

        // Create Alice (user 1)
        alice_ = makeUser(0, "alice", user_engagement::ACTION_HOME);
        alice_id_ = wrapper_->users.createUser("alice", serializeUser(alice_));
        ASSERT_GT(alice_id_, 0);
        alice_.set_user_id(alice_id_);
        wrapper_->users.updateUserProtobuf(alice_id_, serializeUser(alice_));

        // Create Bob (user 2)
        bob_ = makeUser(0, "bob", user_engagement::ACTION_HOME);
        bob_id_ = wrapper_->users.createUser("bob", serializeUser(bob_));
        ASSERT_GT(bob_id_, 0);
        bob_.set_user_id(bob_id_);
        wrapper_->users.updateUserProtobuf(bob_id_, serializeUser(bob_));
    }

    void TearDown() override {
        delete debate_;
        delete wrapper_;
        delete db_;
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
    }

    // ---- Helpers to act as a specific user ----
    void actAsAlice() { userId_ = alice_id_; user_ = &alice_; }
    void actAsBob()   { userId_ = bob_id_;   user_ = &bob_;   }

    void enterDebate(int debateId) {
        debateId_ = debateId;
        MoveUserHandler::EnterDebate(debateId, userId_, *debate_);
        *user_ = debate_->getUserProtobuf(userId_);
    }

    void addChild(int parentId, const std::string& text,
                  const std::string& connection) {
        AddClaimHandler::OpenAddChildClaim(userId_, *debate_);
        AddClaimHandler::AddClaimUnderClaim({text}, {connection}, "", userId_, *debate_);
        *user_ = debate_->getUserProtobuf(userId_);
    }

    void goToClaim(int claimId) {
        MoveUserHandler::GoToClaim(claimId, userId_, *debate_);
        *user_ = debate_->getUserProtobuf(userId_);
    }

    void challengeCurrentClaim(const std::string& reason) {
        // Full challenge event pipeline:
        // 1. Start challenge -> sets CHALLENGING_CLAIM
        ChallengeHandler::StartChallengeClaim(userId_, *debate_);
        // 2. Add current claim to challenge list
        ChallengeHandler::AddClaimToBeChallenged(
            user_->engagement().debating_info().current_claim().id(),
            userId_, *debate_);
        // 3. Open add challenge modal
        ChallengeHandler::OpenAddChallenge(userId_, *debate_);
        // 4. Submit challenge -> creates challenge claim + CHALLENGE link
        ChallengeHandler::SubmitChallengeClaim({reason}, {"challenge claim"}, "", userId_, *debate_);
        *user_ = debate_->getUserProtobuf(userId_);
    }

    void concede() {
        int challengeLinkId = resolveChallengeLinkIdToConcede();
        ChallengeHandler::ConcedeChallenge(userId_, challengeLinkId, *debate_);
        *user_ = debate_->getUserProtobuf(userId_);
    }

    // Test-only stand-in for what a real frontend's UI state would supply
    // directly (the specific challenge link being conceded to). Tries the two
    // scenarios these tests exercise:
    //   1. The conceding user IS the challenger: SubmitChallengeClaim left their
    //      current_claim pointed at the challenge claim, so its own outgoing
    //      CHALLENGE link is the one to concede.
    //   2. The conceding user was challenged: find the CHALLENGE link whose
    //      target claim they created.
    int resolveChallengeLinkIdToConcede() {
        int currentClaimId = user_->engagement().debating_info().current_claim().id();
        debate::Relationship::Link fromCurrent = debate_->findOutgoingChallengeLink(currentClaimId).link();
        if (fromCurrent.link_type() == debate::LinkType::CHALLENGE) {
            return fromCurrent.id();
        }

        int bestFromClaimId = -1;
        int bestLinkId = -1;
        for (const auto& linkTuple : debate_->getLinksForDebate(debateId_)) {
            int linkId = std::get<0>(linkTuple);
            int fromClaimId = std::get<1>(linkTuple);
            int toClaimId = std::get<2>(linkTuple);
            int linkType = std::get<5>(linkTuple);
            if (linkType != static_cast<int>(debate::LinkType::CHALLENGE)) continue;
            debate::Claim targetClaim = debate_->getClaimById(toClaimId);
            if (targetClaim.id() != 0 && targetClaim.creator_id() == userId_) {
                if (fromClaimId > bestFromClaimId) {
                    bestFromClaimId = fromClaimId;
                    bestLinkId = linkId;
                }
            }
        }
        return bestLinkId;
    }

    debate::Claim claim(int id) { return debate_->getClaimById(id); }

    std::string db_path_;
    Database*      db_      = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
    DebateWrapper* debate_  = nullptr;
    int debateId_ = 0;

    int alice_id_ = 0;
    int bob_id_   = 0;
    user::User alice_;
    user::User bob_;

    int userId_ = 0;
    user::User* user_ = nullptr;
};

// ============================================================
// TEST 1: Challenge a leaf claim → challenge claim + link created
// ============================================================
// Debate tree:
//       1 (root: "AI is beneficial")
//      / \
//     2   3
//
// Bob challenges claim 2 → a challenge claim is created with a CHALLENGE link.
// On the refactor branch there is no CHALLENGED status — the target claim
// stays UNDETERMINED until the challenge claim's status is resolved.
// ============================================================
TEST_F(PropagationTest, ChallengeLeaf_CreatesChallengeClaimAndLink) {
    actAsAlice();

    // Alice creates debate
    DebateHandler::AddDebate("AI is beneficial", userId_, *debate_);
    int debateId = 1;

    // Alice enters debate
    enterDebate(debateId);

    // Alice adds two children under root
    addChild(1, "AI improves healthcare", "supports");       // claim 2
    int claim2 = debate_->findChildrenIds(1).back();

    goToClaim(1);  // go back to root
    addChild(1, "AI causes job displacement", "opposes");   // claim 3
    int claim3 = debate_->findChildrenIds(1).back();

    // All claims start as UNDETERMINED
    EXPECT_EQ(claim(1).status(), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(claim(claim2).status(), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(claim(claim3).status(), debate::ClaimStatus::UNDETERMINED);

    // Bob enters debate and challenges claim 2
    actAsBob();
    enterDebate(debateId);
    goToClaim(claim2);

    // Verify Bob is at claim 2
    EXPECT_EQ(user_->engagement().debating_info().current_claim().id(), claim2);

    // Bob submits a challenge
    challengeCurrentClaim("Healthcare AI has bias issues");

    // --- Verify challenge claim and link were created ---

    // Target claim stays UNDETERMINED (no CHALLENGED status on refactor branch)
    EXPECT_EQ(claim(claim2).status(), debate::ClaimStatus::UNDETERMINED)
        << "Challenged claim stays UNDETERMINED until challenge claim is resolved";

    // Unrelated sibling stays UNDETERMINED
    EXPECT_EQ(claim(claim3).status(), debate::ClaimStatus::UNDETERMINED)
        << "Unrelated sibling should remain UNDETERMINED";

    // Verify the challenge claim was created — it should be the highest-ID claim
    auto links = debate_->getLinksForDebate(1);
    bool foundChallengeLink = false;
    for (const auto& link : links) {
        int linkType = std::get<5>(link);  // link_type field
        if (linkType == static_cast<int>(debate::LinkType::CHALLENGE)) {
            foundChallengeLink = true;
            int fromClaim = std::get<1>(link);
            int toClaim   = std::get<2>(link);
            EXPECT_EQ(toClaim, claim2)
                << "CHALLENGE link should point to the challenged claim";
            // The challenge claim should be the from-claim
            EXPECT_NE(fromClaim, claim2)
                << "Challenge claim should be different from challenged claim";
            break;
        }
    }
    EXPECT_TRUE(foundChallengeLink)
        << "A CHALLENGE link should exist after submitting a challenge";
}

// ============================================================
// TEST 3: Challenge a root claim → only root affected
// ============================================================
TEST_F(PropagationTest, ChallengeRoot_OnlyRootAffected) {
    actAsAlice();

    DebateHandler::AddDebate("Root challenge test", userId_, *debate_);
    enterDebate(1);
    addChild(1, "Supporting evidence", "supports");
    int childId = debate_->findChildrenIds(1).back();

    // Bob challenges root
    actAsBob();
    enterDebate(1);
    // Already at root after entering
    challengeCurrentClaim("Root claim is false");

    // Root stays UNDETERMINED (challenge claim is UNDETERMINED by default)
    EXPECT_EQ(claim(1).status(), debate::ClaimStatus::UNDETERMINED);
    // Child stays UNDETERMINED (propagation only goes UP, not DOWN)
    EXPECT_EQ(claim(childId).status(), debate::ClaimStatus::UNDETERMINED)
        << "Child should remain UNDETERMINED — propagation goes UP only";
}

// ============================================================
// TEST 4: Multiple challenges on same claim
// ============================================================
TEST_F(PropagationTest, MultipleChallenges_OnSameClaim) {
    // We need a 3rd user for this test
    user::User charlie = makeUser(0, "charlie", user_engagement::ACTION_HOME);
    int charlie_id = wrapper_->users.createUser("charlie", serializeUser(charlie));
    ASSERT_GT(charlie_id, 0);
    charlie.set_user_id(charlie_id);
    wrapper_->users.updateUserProtobuf(charlie_id, serializeUser(charlie));

    actAsAlice();
    DebateHandler::AddDebate("Multi-challenge test", userId_, *debate_);
    enterDebate(1);
    addChild(1, "Evidence A", "supports");
    int evidenceA = debate_->findChildrenIds(1).back();

    // Bob challenges evidence A once
    actAsBob();
    enterDebate(1);
    goToClaim(evidenceA);
    challengeCurrentClaim("Evidence A is fabricated");

    // Charlie also challenges evidence A
    userId_ = charlie_id;
    user_ = &charlie;
    enterDebate(1);
    goToClaim(evidenceA);
    challengeCurrentClaim("Evidence A is also outdated");

    // There should be 2 CHALLENGE links total
    auto links = debate_->getLinksForDebate(1);
    int challengeCount = 0;
    for (const auto& link : links) {
        if (std::get<5>(link) == static_cast<int>(debate::LinkType::CHALLENGE)) {
            challengeCount++;
        }
    }
    EXPECT_EQ(challengeCount, 2) << "Should have 2 CHALLENGE links";
}

// ============================================================
// TEST 5: Concede challenge → clears user interaction state
// ============================================================
TEST_F(PropagationTest, ConcedeChallenge_ClearsUserState) {
    actAsAlice();
    DebateHandler::AddDebate("Concede test", userId_, *debate_);
    enterDebate(1);
    addChild(1, "Claim to concede", "supports");
    int targetClaim = debate_->findChildrenIds(1).back();

    // Bob challenges it
    actAsBob();
    enterDebate(1);
    goToClaim(targetClaim);
    challengeCurrentClaim("I challenge this");

    // After challenge, Bob's state should be back to VIEWING_CLAIM
    EXPECT_EQ(user_->engagement().debating_info().current_debate_action().action_type(),
              user_engagement::DebatingInfo::CurrentDebateAction::VIEWING_CLAIM);

    // Bob concedes
    concede();

    // State should still be VIEWING_CLAIM (concede only clears interaction state)
    EXPECT_EQ(user_->engagement().debating_info().current_debate_action().action_type(),
              user_engagement::DebatingInfo::CurrentDebateAction::VIEWING_CLAIM);
}

// ============================================================
// Selecting an ARGUMENT in the graph.
//
// Clicking a pill selects the relation and focuses the claim it is for, so
// the panel and the challenge modal -- which read the links under the current
// claim -- always have the selected one in hand. From there one click puts it
// into a challenge: the modal opens with the argument already picked.
// ============================================================
TEST_F(PropagationTest, SelectingALinkFocusesItsClaimAndOneClickStartsAnUndercut) {
    actAsAlice();
    DebateHandler::AddDebate("Speed limits save lives", userId_, *debate_);
    enterDebate(1);
    addChild(1, "Crash deaths fell 23 percent", "");
    const int child = debate_->findChildrenIds(1).back();
    const std::vector<int> links = debate_->findLinksUnder(1);
    ASSERT_EQ(links.size(), 1u);

    // Bob is standing on the child when he clicks the pill above it.
    actAsBob();
    enterDebate(1);
    goToClaim(child);
    MoveUserHandler::SelectLink(links[0], userId_, *debate_);
    *user_ = debate_->getUserProtobuf(userId_);
    EXPECT_EQ(user_->engagement().debating_info().current_claim().id(), 1)
        << "selecting the argument focuses the claim it is for";
    EXPECT_EQ(user_->engagement().debating_info().selected_link_id(), links[0]);

    // The panel's one button.
    ChallengeHandler::AddLinkToBeChallenged(links[0], userId_, *debate_);
    *user_ = debate_->getUserProtobuf(userId_);
    const auto& debating = user_->engagement().debating_info();
    EXPECT_EQ(debating.current_debate_action().action_type(),
              user_engagement::DebatingInfo::CurrentDebateAction::CHALLENGING_CLAIM);
    EXPECT_TRUE(debating.challenging_info().opened_challenge_modal());
    ASSERT_EQ(debating.challenging_info().link_ids_size(), 1);
    EXPECT_EQ(debating.challenging_info().link_ids(0), links[0]);

    // Adding a link while already challenging does not restart anything.
    ChallengeHandler::AddLinkToBeChallenged(links[0], userId_, *debate_);
    *user_ = debate_->getUserProtobuf(userId_);
    EXPECT_EQ(user_->engagement().debating_info().challenging_info().link_ids_size(), 2);

    // A claim selection ends the argument selection.
    ChallengeHandler::CancelChallengeClaim(userId_, *debate_);
    MoveUserHandler::SelectClaim(child, userId_, *debate_);
    *user_ = debate_->getUserProtobuf(userId_);
    EXPECT_EQ(user_->engagement().debating_info().selected_link_id(), 0);
    EXPECT_EQ(user_->engagement().debating_info().current_claim().id(), child);

    // An unknown link selects nothing and leaves the cursor alone.
    MoveUserHandler::SelectLink(9999, userId_, *debate_);
    *user_ = debate_->getUserProtobuf(userId_);
    EXPECT_EQ(user_->engagement().debating_info().selected_link_id(), 0);
    EXPECT_EQ(user_->engagement().debating_info().current_claim().id(), child);
}
