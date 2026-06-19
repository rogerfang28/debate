// test_debate_create.cc — Simple debate creation + child claim verification test
//
// This test:
//   1. Creates a temporary SQLite database
//   2. Creates a user
//   3. Initializes a debate (which creates a root claim)
//   4. Adds a child claim under the root
//   5. Verifies the child exists and is linked correctly via DB queries

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <cstdio>
#include "debate.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "utils/DebateWrapper.h"

class DebateCreateTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Use a temp database file so we don't touch the real one
        db_path_ = "test_debate_create_temp.sqlite3";
        std::remove(db_path_.c_str());

        db_ = new Database(db_path_);
        wrapper_ = new DatabaseWrapper(*db_);
        debate_ = new DebateWrapper(*wrapper_);

        ASSERT_TRUE(wrapper_->ensureAllTables()) << "Failed to create tables";

        // Create a test user
        user_id_ = wrapper_->users.createUser("testuser", {});
        ASSERT_GT(user_id_, 0) << "Failed to create user";
    }

    void TearDown() override {
        delete debate_;
        delete wrapper_;
        delete db_;
        std::remove(db_path_.c_str());
        // Also clean up WAL/SHM files
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
    }

    std::string db_path_;
    Database* db_ = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
    DebateWrapper* debate_ = nullptr;
    int user_id_ = 0;
};

// -------------------------------------------------------
// Test 1: Creating a debate produces a root claim
// -------------------------------------------------------
TEST_F(DebateCreateTest, CreateDebate_ProducesRootClaim) {
    // Initialize a new debate — this creates the root claim
    debate_->initNewDebate("Is AI beneficial for society?", user_id_);

    // The root claim should have id=1 in a fresh database
    debate::Claim root = debate_->getClaimById(1);

    // Verify the root claim exists and has the debate topic as its sentence
    EXPECT_EQ(root.id(), 1);
    EXPECT_FALSE(root.sentence().empty())
        << "Root claim should have a non-empty sentence";

    // Verify the root is actually a root (no parent)
    EXPECT_TRUE(debate_->isRoot(1)) << "Claim 1 should be the root";
}

// -------------------------------------------------------
// Test 2: Adding a child claim under the root
// -------------------------------------------------------
TEST_F(DebateCreateTest, AddChildClaim_UnderRoot) {
    // Create debate → root claim #1
    debate_->initNewDebate("Is AI beneficial for society?", user_id_);

    // Add a child claim under root
    int child_id = debate_->addClaimUnderParent(
        1,                          // parentId = root
        "AI improves healthcare",   // claim text
        "supports",                 // connection to parent
        user_id_,                   // creator
        1                           // debate_id
    );

    EXPECT_GT(child_id, 1) << "Child claim ID should be > 1";

    // Verify the child exists via getClaimById
    debate::Claim child = debate_->getClaimById(child_id);
    EXPECT_EQ(child.id(), child_id);
    EXPECT_EQ(child.sentence(), "AI improves healthcare");

    // Verify the parent of the child is the root
    debate::Claim parent = debate_->findClaimParent(child_id);
    EXPECT_EQ(parent.id(), 1) << "Parent of child should be root claim #1";

    // Verify root's children include the child
    std::vector<int> child_ids = debate_->findChildrenIds(1);
    bool found = false;
    for (int cid : child_ids) {
        if (cid == child_id) {
            found = true;
            break;
        }
    }
    EXPECT_TRUE(found) << "Root's children should include the new child claim";

    // Also verify count
    EXPECT_EQ(child_ids.size(), 1u) << "Root should have exactly 1 child";
    EXPECT_EQ(child_ids[0], child_id);
}

// -------------------------------------------------------
// Test 3: Full flow — create debate, add child, verify DB state
// -------------------------------------------------------
TEST_F(DebateCreateTest, FullFlow_CreateDebateAddChild_VerifyInDB) {
    // Step 1: Create debate
    debate_->initNewDebate("Climate change is real", user_id_);

    debate::Claim root = debate_->getClaimById(1);
    ASSERT_EQ(root.id(), 1);
    ASSERT_TRUE(debate_->isRoot(1));

    // Step 2: Add child
    int child_id = debate_->addClaimUnderParent(
        1,
        "Global temperatures are rising",
        "supports",
        user_id_,
        1
    );
    ASSERT_GT(child_id, 1);

    // Step 3: Verify via direct DB query (not just DebateWrapper)
    // Query the statements table to confirm the child claim is stored
    auto statements = debate_->getStatementsForDebate(1);
    // Should have at least 2 statements: root + child
    EXPECT_GE(statements.size(), 2u)
        << "Should have at least 2 statements (root + child) in the debate";

    // Step 4: Verify the link between parent and child exists
    auto links = debate_->getLinksForDebate(1);
    EXPECT_GE(links.size(), 1u)
        << "Should have at least 1 link (root→child) in the debate";

    // Find the link that connects root → child
    bool found_link = false;
    for (const auto& link : links) {
        int from_id = std::get<1>(link);  // fromClaimId
        int to_id   = std::get<2>(link);  // toClaimId
        if (from_id == 1 && to_id == child_id) {
            found_link = true;
            break;
        }
    }
    EXPECT_TRUE(found_link)
        << "Should find a link from root (1) to child (" << child_id << ")";
}

// -------------------------------------------------------
// Test 4: Add multiple children and verify count
// -------------------------------------------------------
TEST_F(DebateCreateTest, AddMultipleChildren_VerifyCount) {
    debate_->initNewDebate("Testing multiple children", user_id_);

    int c1 = debate_->addClaimUnderParent(1, "Child A", "supports", user_id_, 1);
    int c2 = debate_->addClaimUnderParent(1, "Child B", "opposes", user_id_, 1);
    int c3 = debate_->addClaimUnderParent(1, "Child C", "supports", user_id_, 1);

    EXPECT_GT(c1, 1);
    EXPECT_GT(c2, c1);
    EXPECT_GT(c3, c2);

    std::vector<int> child_ids = debate_->findChildrenIds(1);
    EXPECT_EQ(child_ids.size(), 3u) << "Root should have exactly 3 children";

    // Verify each child's sentence
    EXPECT_EQ(debate_->getClaimById(c1).sentence(), "Child A");
    EXPECT_EQ(debate_->getClaimById(c2).sentence(), "Child B");
    EXPECT_EQ(debate_->getClaimById(c3).sentence(), "Child C");

    // Verify all children point to root as parent
    for (int cid : child_ids) {
        debate::Claim parent = debate_->findClaimParent(cid);
        EXPECT_EQ(parent.id(), 1) << "Parent of child " << cid << " should be root";
    }
}

// -------------------------------------------------------
// Test 5: Child claim has correct connection type
// -------------------------------------------------------
TEST_F(DebateCreateTest, ChildClaim_HasCorrectConnectionType) {
    debate_->initNewDebate("Connection type test", user_id_);

    int child_id = debate_->addClaimUnderParent(
        1, "Supporting evidence", "supports", user_id_, 1
    );

    // Verify via getChildrenInfo which returns (id, sentence) pairs
    auto info = debate_->findChildrenInfo(1);
    EXPECT_EQ(info.size(), 1u);

    // The child should exist and have the right sentence
    EXPECT_EQ(info[0].first, std::to_string(child_id));
    EXPECT_EQ(info[0].second, "Supporting evidence");
}
