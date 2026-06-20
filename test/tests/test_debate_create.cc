// test_debate_create.cc — Event-driven debate creation + child claim test
//
// These tests simulate the FULL event pipeline that the virtual renderer uses.
// Instead of calling DebateWrapper / DatabaseWrapper directly, we:
//   1. Set up user engagement state (protobuf) just like the VR would
//   2. Call handler functions (DebateHandler, MoveUserHandler, AddClaimHandler)
//      which is what the DebateModerator dispatches to
//   3. Verify the results through database queries
//
// This ensures the tests are always consistent with how the frontend actually
// interacts with the backend — same code paths, same side effects.

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdio>

#include "debate.pb.h"
#include "user.pb.h"
#include "user_engagement.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "utils/DebateWrapper.h"

// Handlers — the same ones DebateModerator dispatches to
#include "debateModerator/event-handlers/DebateHandler/DebateHandler.h"
#include "debateModerator/event-handlers/MoveUserHandler/MoveUserHandler.h"
#include "debateModerator/event-handlers/AddClaimHandler/AddClaimHandler.h"

// -------------------------------------------------------
// Helper: Build a default user engagement protobuf
// -------------------------------------------------------
static user::User makeUserEngagement(int userId, int currentAction) {
    user::User u;
    u.set_user_id(userId);
    u.set_username("user_" + std::to_string(userId));
    u.mutable_engagement()->set_current_action(
        static_cast<user_engagement::EngagementAction>(currentAction));
    return u;
}

// -------------------------------------------------------
// Test fixture
// -------------------------------------------------------
class DebateEventTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "test_event_temp.sqlite3";
        std::remove(db_path_.c_str());

        db_      = new Database(db_path_);
        wrapper_ = new DatabaseWrapper(*db_);
        debate_  = new DebateWrapper(*wrapper_);

        ASSERT_TRUE(wrapper_->ensureAllTables()) << "Failed to create tables";

        // Create a user with a stored engagement protobuf so handlers can read it
        userProto_ = makeUserEngagement(0 /* will be replaced */, user_engagement::ACTION_HOME);
        std::vector<uint8_t> serialized(userProto_.ByteSizeLong());
        userProto_.SerializeToArray(serialized.data(), serialized.size());

        user_id_ = wrapper_->users.createUser("testuser", serialized);
        ASSERT_GT(user_id_, 0) << "Failed to create user";

        // Update the proto with the real ID
        userProto_.set_user_id(user_id_);
        serialized.resize(userProto_.ByteSizeLong());
        userProto_.SerializeToArray(serialized.data(), serialized.size());
        wrapper_->users.updateUserProtobuf(user_id_, serialized);
    }

    void TearDown() override {
        delete debate_;
        delete wrapper_;
        delete db_;
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
    }

    // Helper: re-read user engagement from DB
    user::User refreshUser() {
        return debate_->getUserProtobuf(user_id_);
    }

    std::string    db_path_;
    Database*      db_      = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
    DebateWrapper* debate_  = nullptr;
    int            user_id_ = 0;
    user::User     userProto_;
};

// -------------------------------------------------------
// Test 1: Create a debate via DebateHandler (event path)
// -------------------------------------------------------
TEST_F(DebateEventTest, CreateDebate_ViaHandler) {
    // The handler just calls debateWrapper.initNewDebate(topic, user_id)
    DebateHandler::AddDebate("Is AI beneficial?", user_id_, *debate_);

    // Verify: root claim exists
    debate::Claim root = debate_->getClaimById(1);
    ASSERT_EQ(root.id(), 1);
    EXPECT_EQ(root.sentence(), "Is AI beneficial?");
    EXPECT_TRUE(debate_->isRoot(1));
}

// -------------------------------------------------------
// Test 2: Full event flow — create debate → enter → add child
// -------------------------------------------------------
TEST_F(DebateEventTest, FullEventFlow_CreateDebate_Enter_AddChild) {
    // Step 1: Create debate via handler (event: CREATE_DEBATE)
    DebateHandler::AddDebate("Is AI beneficial?", user_id_, *debate_);

    // Step 2: Enter the debate (event: ENTER_DEBATE)
    // This sets user engagement: current_action=ACTION_DEBATING, debate_id, root_claim
    bool entered = MoveUserHandler::EnterDebate(1, user_id_, *debate_);
    EXPECT_TRUE(entered) << "EnterDebate should succeed";

    // Verify user is now "in" the debate
    user::User u = refreshUser();
    EXPECT_EQ(u.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(u.engagement().debating_info().debate_id(), 1);
    EXPECT_EQ(u.engagement().debating_info().current_claim().id(), 1); // at root

    // Step 3: Open add child claim (event: OPEN_ADD_CHILD_CLAIM)
    // Sets current_debate_action to ADDING_CHILD_CLAIM
    AddClaimHandler::OpenAddChildClaim(user_id_, *debate_);

    u = refreshUser();
    EXPECT_EQ(u.engagement().debating_info().current_debate_action().action_type(),
              user_engagement::DebatingInfo::CurrentDebateAction::ADDING_CHILD_CLAIM);

    // Step 4: Submit the child claim (event: SUBMIT_CLAIM)
    // Reads current_claim_id and debate_id from user engagement, adds claim
    AddClaimHandler::AddClaimUnderClaim(
        "AI improves healthcare",   // claim text
        "supports",                 // connection to parent
        user_id_,
        *debate_
    );

    // Step 5: Verify via database queries
    // The child should exist
    std::vector<int> childIds = debate_->findChildrenIds(1);
    ASSERT_EQ(childIds.size(), 1u) << "Root should have exactly 1 child";

    int childId = childIds[0];
    debate::Claim child = debate_->getClaimById(childId);
    EXPECT_EQ(child.sentence(), "AI improves healthcare");

    // Parent of child should be root
    debate::Claim parent = debate_->findClaimParent(childId);
    EXPECT_EQ(parent.id(), 1);

    // Verify statements table has 2 entries (root + child)
    auto statements = debate_->getStatementsForDebate(1);
    EXPECT_GE(statements.size(), 2u);

    // Verify links table has the parent-child link
    auto links = debate_->getLinksForDebate(1);
    EXPECT_GE(links.size(), 1u);

    // After adding, handler should have closed the add-child state
    u = refreshUser();
    EXPECT_EQ(u.engagement().debating_info().current_debate_action().action_type(),
              user_engagement::DebatingInfo::CurrentDebateAction::VIEWING_CLAIM);
}

// -------------------------------------------------------
// Test 3: Add multiple children through the event pipeline
// -------------------------------------------------------
TEST_F(DebateEventTest, AddMultipleChildren_ViaEventPipeline) {
    // Setup: create debate + enter
    DebateHandler::AddDebate("Testing multiple children", user_id_, *debate_);
    MoveUserHandler::EnterDebate(1, user_id_, *debate_);

    // Add 3 children through the full event pipeline
    int childIds[3];
    const char* sentences[] = {"Child A", "Child B", "Child C"};
    const char* connections[] = {"supports", "opposes", "supports"};

    for (int i = 0; i < 3; i++) {
        AddClaimHandler::OpenAddChildClaim(user_id_, *debate_);
        AddClaimHandler::AddClaimUnderClaim(sentences[i], connections[i], user_id_, *debate_);
    }

    // Verify all 3 children exist
    std::vector<int> found = debate_->findChildrenIds(1);
    EXPECT_EQ(found.size(), 3u);

    for (int i = 0; i < 3; i++) {
        debate::Claim c = debate_->getClaimById(found[i]);
        EXPECT_EQ(c.sentence(), sentences[i]);

        debate::Claim p = debate_->findClaimParent(found[i]);
        EXPECT_EQ(p.id(), 1) << "Parent of child " << found[i] << " should be root";
    }
}

// -------------------------------------------------------
// Test 4: Navigate to a child, then add a grandchild
// -------------------------------------------------------
TEST_F(DebateEventTest, NavigateAndAddGrandchild_ViaEventPipeline) {
    // Setup: create debate + enter
    DebateHandler::AddDebate("Grandchild test", user_id_, *debate_);
    MoveUserHandler::EnterDebate(1, user_id_, *debate_);

    // Add a child under root
    AddClaimHandler::OpenAddChildClaim(user_id_, *debate_);
    AddClaimHandler::AddClaimUnderClaim("Child claim", "supports", user_id_, *debate_);

    int childId = debate_->findChildrenIds(1)[0];

    // Navigate to the child claim (event: GO_TO_CLAIM)
    MoveUserHandler::GoToClaim(childId, user_id_, *debate_);

    user::User u = refreshUser();
    EXPECT_EQ(u.engagement().debating_info().current_claim().id(), childId);

    // Add a grandchild under the child
    AddClaimHandler::OpenAddChildClaim(user_id_, *debate_);
    AddClaimHandler::AddClaimUnderClaim("Grandchild claim", "supports", user_id_, *debate_);

    // Verify grandchild exists and is under the child (not root)
    std::vector<int> rootChildren = debate_->findChildrenIds(1);
    EXPECT_EQ(rootChildren.size(), 1u) << "Root should still have exactly 1 child";

    std::vector<int> childChildren = debate_->findChildrenIds(childId);
    EXPECT_EQ(childChildren.size(), 1u) << "Child should have exactly 1 child (grandchild)";

    debate::Claim grandchild = debate_->getClaimById(childChildren[0]);
    EXPECT_EQ(grandchild.sentence(), "Grandchild claim");

    debate::Claim gp = debate_->findClaimParent(childChildren[0]);
    EXPECT_EQ(gp.id(), childId) << "Parent of grandchild should be the child claim";
}

// -------------------------------------------------------
// Test 5: User engagement state transitions are correct
// -------------------------------------------------------
TEST_F(DebateEventTest, UserEngagement_StateTransitions) {
    // Initially: ACTION_HOME
    user::User u = refreshUser();
    EXPECT_EQ(u.engagement().current_action(), user_engagement::ACTION_HOME);

    // After creating debate: still HOME (AddDebate doesn't change user state)
    DebateHandler::AddDebate("State test", user_id_, *debate_);
    u = refreshUser();
    EXPECT_EQ(u.engagement().current_action(), user_engagement::ACTION_HOME);

    // After entering debate: ACTION_DEBATING
    MoveUserHandler::EnterDebate(1, user_id_, *debate_);
    u = refreshUser();
    EXPECT_EQ(u.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(u.engagement().debating_info().debate_id(), 1);

    // After OpenAddChildClaim: current_debate_action = ADDING_CHILD_CLAIM
    AddClaimHandler::OpenAddChildClaim(user_id_, *debate_);
    u = refreshUser();
    EXPECT_EQ(u.engagement().debating_info().current_debate_action().action_type(),
              user_engagement::DebatingInfo::CurrentDebateAction::ADDING_CHILD_CLAIM);

    // After AddClaimUnderClaim: back to VIEWING_CLAIM (CloseAddChildClaim called)
    AddClaimHandler::AddClaimUnderClaim("Test claim", "supports", user_id_, *debate_);
    u = refreshUser();
    EXPECT_EQ(u.engagement().debating_info().current_debate_action().action_type(),
              user_engagement::DebatingInfo::CurrentDebateAction::VIEWING_CLAIM);

    // After GoToClaim: current_claim changes
    int childId = debate_->findChildrenIds(1)[0];
    MoveUserHandler::GoToClaim(childId, user_id_, *debate_);
    u = refreshUser();
    EXPECT_EQ(u.engagement().debating_info().current_claim().id(), childId);

    // After GoHome: back to ACTION_HOME
    MoveUserHandler::GoHome(user_id_, *debate_);
    u = refreshUser();
    EXPECT_EQ(u.engagement().current_action(), user_engagement::ACTION_HOME);
}
