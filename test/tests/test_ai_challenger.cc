// test_ai_challenger.cc -- placing AI-proposed challenges
//
// Written to settle a specific question: a live run produced five challenges
// whose stored basis was the "challenge link" placeholder, meaning the basis
// arrived empty. Either the model omitted it or this code dropped it, and
// guessing between those two is how you fix the wrong one.

#include <gtest/gtest.h>

#include <cstdio>
#include <string>

#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "debateModerator/DebateModerator.h"
#include "utils/AiChallenger.h"
#include "utils/DebateWrapper.h"
#include "utils/GeneratedDebateBuilder.h"
#include "utils/RelationBuilder.h"

#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

class AiChallengerTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = std::string("test_chaldb_") + std::to_string(GETPID()) + ".sqlite3";
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
        // DebateModerator opens the DEBATES database itself from DB_PATH.
        _putenv_s("DB_PATH", db_path_.c_str());
        db_ = new Database(db_path_);
        dbw_ = new DatabaseWrapper(*db_, *db_);
        ASSERT_TRUE(dbw_->ensureAllTables());
        moderator_ = new DebateModerator(*db_);
        wrapper_ = &moderator_->getDebateWrapper();
        user_id_ = moderator_->createUserIfNotExist("challenger");

        // A small real debate to attack.
        const std::string json = R"({
          "root": {"text": "Remote work raises productivity", "description": ""},
          "arguments": [
            {"target": "root", "kind": "supports", "basis": "EVIDENTIAL",
             "claims": [{"id": "c1", "text": "Output per engineer rose", "description": ""}]}
          ]
        })";
        const auto built = GeneratedDebateBuilder::Build(json, user_id_, *moderator_);
        ASSERT_TRUE(built.ok) << built.error;
        debate_id_ = built.debateId;
        for (const auto& blob : wrapper_->getStatementsForDebate(debate_id_)) {
            debate::Claim claim;
            ASSERT_TRUE(claim.ParseFromArray(blob.data(), static_cast<int>(blob.size())));
            if (claim.sentence() == "Output per engineer rose") {
                target_id_ = claim.id();
            }
        }
        ASSERT_GT(target_id_, 0);
    }

    void TearDown() override {
        delete moderator_;
        delete dbw_;
        delete db_;
        _putenv_s("DB_PATH", "");
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
    }

    std::vector<debate::Relationship> relations() {
        return RelationBuilder::fromLinkRows(
            wrapper_->getLinksForDebateAndCreators(debate_id_, {user_id_}), debate_id_);
    }

    std::string db_path_;
    Database* db_ = nullptr;
    DatabaseWrapper* dbw_ = nullptr;
    DebateModerator* moderator_ = nullptr;
    DebateWrapper* wrapper_ = nullptr;
    int user_id_ = 0;
    int debate_id_ = 0;
    int target_id_ = 0;
};

// THE QUESTION THIS FILE WAS WRITTEN FOR. If the basis survives here, the live
// run lost it because the model never sent one.
TEST_F(AiChallengerTest, TheBasisSurvivesOntoTheRelation) {
    const std::string json = R"({"challenges": [{"target_claim_id": )" +
                             std::to_string(target_id_) +
                             R"(, "text": "The measure counts commits, not shipped work",
                                 "description": "It measures activity, not output",
                                 "basis": "EVIDENTIAL"}]})";

    const auto result = AiChallenger::Apply(json, user_id_, *moderator_);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.challengesAdded, 1);

    bool found = false;
    for (const auto& relation : relations()) {
        if (relation.role() == debate::OPPOSES) {
            found = true;
            EXPECT_EQ(relation.basis(), "EVIDENTIAL")
                << "an empty basis here would mean this code drops it, not the model";
        }
    }
    EXPECT_TRUE(found) << "the challenge should exist as an opposing relation";
}

// Placing an objection on the wrong sentence puts words into someone's
// argument, so an unknown target is dropped rather than guessed at.
TEST_F(AiChallengerTest, ChallengesNamingAnUnknownClaimAreSkippedNotPlaced) {
    const std::string json =
        R"({"challenges": [{"target_claim_id": 999999, "text": "Nowhere to put this", "basis": ""}]})";

    const auto result = AiChallenger::Apply(json, user_id_, *moderator_);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.challengesAdded, 0);
    EXPECT_EQ(result.skipped, 1);
    for (const auto& relation : relations()) {
        EXPECT_NE(relation.role(), debate::OPPOSES) << "nothing should have been placed";
    }
}

TEST_F(AiChallengerTest, AChallengeWithNoTextIsSkipped) {
    const std::string json = R"({"challenges": [{"target_claim_id": )" +
                             std::to_string(target_id_) + R"(, "text": "", "basis": "EVIDENTIAL"}]})";

    const auto result = AiChallenger::Apply(json, user_id_, *moderator_);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.challengesAdded, 0);
    EXPECT_EQ(result.skipped, 1);
}

TEST_F(AiChallengerTest, AnEmptyChallengeListIsSuccessNotFailure) {
    const auto result = AiChallenger::Apply(R"({"challenges": []})", user_id_, *moderator_);
    EXPECT_TRUE(result.ok) << "finding nothing worth challenging is a real answer";
    EXPECT_EQ(result.challengesAdded, 0);
}

TEST_F(AiChallengerTest, MalformedRepliesAreReportedRatherThanApplied) {
    EXPECT_FALSE(AiChallenger::Apply("not json", user_id_, *moderator_).ok);
    EXPECT_FALSE(AiChallenger::Apply(R"({"nope": 1})", user_id_, *moderator_).ok);
}

TEST_F(AiChallengerTest, AppliedChallengesAreAttributedToTheUserAndLogged) {
    const std::string json = R"({"challenges": [{"target_claim_id": )" +
                             std::to_string(target_id_) +
                             R"(, "text": "The sample was too small", "basis": "EVIDENTIAL"}]})";
    const auto movesBefore = wrapper_->getMovesForDebate(debate_id_).size();

    ASSERT_TRUE(AiChallenger::Apply(json, user_id_, *moderator_).ok);

    EXPECT_GT(wrapper_->getMovesForDebate(debate_id_).size(), movesBefore)
        << "an AI challenge is a move like any other";
    for (const auto& blob : wrapper_->getStatementsForDebate(debate_id_)) {
        debate::Claim claim;
        ASSERT_TRUE(claim.ParseFromArray(blob.data(), static_cast<int>(blob.size())));
        EXPECT_EQ(claim.creator_id(), user_id_);
    }
}
