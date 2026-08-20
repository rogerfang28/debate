// test_generated_debate_builder.cc -- generated JSON becomes a real debate
//
// The generator's output is a model's opinion; the builder is where it becomes
// claims and relations that everything else has to live with. So the builder is
// the half worth testing hard, and it is testable without a model at all --
// hand-written JSON goes in, real claims come out.
//
// Two properties matter most here. Bad JSON must not leave a half-built debate
// behind for the user to find and delete. And an unjustified group must be
// split, because grouping declares claims JOINT and the model groups by
// default -- three prompt revisions did not stop it.

#include <gtest/gtest.h>

#include <cstdio>
#include <set>
#include <string>

#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "utils/DebateWrapper.h"
#include "utils/GeneratedDebateBuilder.h"
#include "debateModerator/DebateModerator.h"
#include "utils/RelationBuilder.h"

#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

class GeneratedDebateBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = std::string("test_gendb_") + std::to_string(GETPID()) + ".sqlite3";
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
        // DebateModerator opens the DEBATES database itself from DB_PATH -- it
        // is handed only the users one. Without this it quietly writes to the
        // default path instead of the temp database, and the test both passes
        // against the wrong data and leaves a stray file behind.
        _putenv_s("DB_PATH", db_path_.c_str());
        db_ = new Database(db_path_);
        dbw_ = new DatabaseWrapper(*db_, *db_);
        ASSERT_TRUE(dbw_->ensureAllTables());
        moderator_ = new DebateModerator(*db_);
        wrapper_ = &moderator_->getDebateWrapper();
        user_id_ = moderator_->createUserIfNotExist("generator");
        ASSERT_GT(user_id_, 0);
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

    int countClaims(int debateId) {
        return static_cast<int>(wrapper_->getStatementsForDebate(debateId).size());
    }

    std::string db_path_;
    Database* db_ = nullptr;
    DatabaseWrapper* dbw_ = nullptr;
    DebateModerator* moderator_ = nullptr;
    DebateWrapper* wrapper_ = nullptr;
    int user_id_ = 0;
};

TEST_F(GeneratedDebateBuilderTest, BuildsARootWithOneSupportingClaim) {
    const std::string json = R"({
      "root": {"text": "Remote work raises productivity", "description": ""},
      "arguments": [
        {"target": "root", "kind": "supports", "basis": "EVIDENTIAL",
         "claims": [{"id": "c1", "text": "Commute time is eliminated", "description": "54 minutes daily"}]}
      ]
    })";

    const auto result = GeneratedDebateBuilder::Build(json, user_id_, *moderator_);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_GT(result.debateId, 0);
    EXPECT_EQ(result.claimsCreated, 2) << "the root plus one supporting claim";
    EXPECT_EQ(countClaims(result.debateId), 2);
}

// The distinction the whole tool is built around: several claims in one
// argument are ONE inference, not several independent reasons.
TEST_F(GeneratedDebateBuilderTest, AJustifiedGroupBecomesOneJointRelation) {
    const std::string json = R"({
      "root": {"text": "Prices rose because supply fell", "description": ""},
      "arguments": [
        {"target": "root", "kind": "supports", "basis": "EVIDENTIAL",
         "joint_reason": "the shortage only explains the price move if demand held steady",
         "claims": [
           {"id": "c1", "text": "Supply fell", "description": ""},
           {"id": "c2", "text": "Demand held steady", "description": ""}
         ]}
      ]
    })";

    const auto result = GeneratedDebateBuilder::Build(json, user_id_, *moderator_);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.groupsSplit, 0) << "a justified group must survive intact";
    EXPECT_EQ(result.argumentsCreated, 1);

    const auto relations = RelationBuilder::fromLinkRows(
        wrapper_->getLinksForDebateAndCreators(result.debateId, {user_id_}), result.debateId);
    ASSERT_EQ(relations.size(), 1u) << "two joint claims are ONE relation, not two";
    EXPECT_EQ(relations[0].from_claim_ids_size(), 2);
}

// The model groups by default and omits the justification. Splitting is the
// conservative reading: independent reasons survive each other.
TEST_F(GeneratedDebateBuilderTest, AnUnjustifiedGroupIsSplitIntoIndependentArguments) {
    const std::string json = R"({
      "root": {"text": "The trial went well", "description": ""},
      "arguments": [
        {"target": "root", "kind": "supports", "basis": "EVIDENTIAL",
         "claims": [
           {"id": "c1", "text": "Revenue rose", "description": ""},
           {"id": "c2", "text": "Turnover fell", "description": ""},
           {"id": "c3", "text": "Sick days fell", "description": ""}
         ]}
      ]
    })";

    const auto result = GeneratedDebateBuilder::Build(json, user_id_, *moderator_);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.groupsSplit, 1);
    EXPECT_EQ(result.argumentsCreated, 3) << "three independent reasons, three arguments";

    const auto relations = RelationBuilder::fromLinkRows(
        wrapper_->getLinksForDebateAndCreators(result.debateId, {user_id_}), result.debateId);
    ASSERT_EQ(relations.size(), 3u);
    for (const auto& relation : relations) {
        EXPECT_EQ(relation.from_claim_ids_size(), 1)
            << "an unjustified group must not survive as a joint relation";
    }
}

TEST_F(GeneratedDebateBuilderTest, ChallengesAreBuiltAsOpposingRelations) {
    const std::string json = R"({
      "root": {"text": "Remote work raises productivity", "description": ""},
      "arguments": [
        {"target": "root", "kind": "supports", "basis": "",
         "claims": [{"id": "c1", "text": "Output per engineer rose", "description": ""}]},
        {"target": "c1", "kind": "challenges", "basis": "EVIDENTIAL",
         "claims": [{"id": "c2", "text": "The measure counted commits, not shipped work", "description": ""}]}
      ]
    })";

    const auto result = GeneratedDebateBuilder::Build(json, user_id_, *moderator_);
    ASSERT_TRUE(result.ok) << result.error;

    const auto relations = RelationBuilder::fromLinkRows(
        wrapper_->getLinksForDebateAndCreators(result.debateId, {user_id_}), result.debateId);
    int opposes = 0;
    int supports = 0;
    for (const auto& relation : relations) {
        if (relation.role() == debate::OPPOSES) opposes++;
        if (relation.role() == debate::SUPPORTS) supports++;
    }
    EXPECT_EQ(supports, 1);
    EXPECT_EQ(opposes, 1) << "a challenge must land as opposition, not as another support";
}

TEST_F(GeneratedDebateBuilderTest, ClaimsCanTargetClaimsIntroducedEarlier) {
    const std::string json = R"({
      "root": {"text": "A", "description": ""},
      "arguments": [
        {"target": "root", "kind": "supports", "basis": "", "claims": [{"id": "c1", "text": "B", "description": ""}]},
        {"target": "c1", "kind": "supports", "basis": "", "claims": [{"id": "c2", "text": "C", "description": ""}]},
        {"target": "c2", "kind": "supports", "basis": "", "claims": [{"id": "c3", "text": "D", "description": ""}]}
      ]
    })";

    const auto result = GeneratedDebateBuilder::Build(json, user_id_, *moderator_);
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.claimsCreated, 4) << "a three-deep chain plus the root";
}

// A debate half built from bad JSON is worse than none: the user has to find
// and delete it, and it looks like a real debate until they read it.
TEST_F(GeneratedDebateBuilderTest, RejectsAForwardReferenceWithoutCreatingAnything) {
    const std::string json = R"({
      "root": {"text": "A", "description": ""},
      "arguments": [
        {"target": "c2", "kind": "supports", "basis": "", "claims": [{"id": "c1", "text": "B", "description": ""}]},
        {"target": "root", "kind": "supports", "basis": "", "claims": [{"id": "c2", "text": "C", "description": ""}]}
      ]
    })";

    const auto result = GeneratedDebateBuilder::Build(json, user_id_, *moderator_);
    EXPECT_FALSE(result.ok);
    EXPECT_NE(result.error.find("c2"), std::string::npos) << "the error should name the bad target";
    EXPECT_TRUE(wrapper_->getUserDebateIds(user_id_).empty())
        << "validation must happen before anything is created";
}

TEST_F(GeneratedDebateBuilderTest, RejectsDuplicateClaimIds) {
    const std::string json = R"({
      "root": {"text": "A", "description": ""},
      "arguments": [
        {"target": "root", "kind": "supports", "basis": "", "claims": [{"id": "c1", "text": "B", "description": ""}]},
        {"target": "root", "kind": "supports", "basis": "", "claims": [{"id": "c1", "text": "C", "description": ""}]}
      ]
    })";

    const auto result = GeneratedDebateBuilder::Build(json, user_id_, *moderator_);
    EXPECT_FALSE(result.ok) << "a reused id makes later targets ambiguous";
    EXPECT_TRUE(wrapper_->getUserDebateIds(user_id_).empty());
}

TEST_F(GeneratedDebateBuilderTest, RejectsMalformedOrEmptyInput) {
    EXPECT_FALSE(GeneratedDebateBuilder::Build("not json at all", user_id_, *moderator_).ok);
    EXPECT_FALSE(GeneratedDebateBuilder::Build(R"({"arguments": []})", user_id_, *moderator_).ok)
        << "no root claim";
    EXPECT_FALSE(GeneratedDebateBuilder::Build(
                     R"({"root": {"text": "A"}, "arguments": []})", user_id_, *moderator_).ok)
        << "a root with nothing argued about it is not a debate";
    EXPECT_TRUE(wrapper_->getUserDebateIds(user_id_).empty());
}

TEST_F(GeneratedDebateBuilderTest, RejectsAClaimWithNoText) {
    const std::string json = R"({
      "root": {"text": "A", "description": ""},
      "arguments": [
        {"target": "root", "kind": "supports", "basis": "", "claims": [{"id": "c1", "text": "", "description": ""}]}
      ]
    })";

    EXPECT_FALSE(GeneratedDebateBuilder::Build(json, user_id_, *moderator_).ok);
    EXPECT_TRUE(wrapper_->getUserDebateIds(user_id_).empty());
}

// Generated claims must be indistinguishable from typed ones -- same author,
// same move log -- or every reader downstream needs a special case.
TEST_F(GeneratedDebateBuilderTest, GeneratedClaimsBelongToTheUserAndAreLogged) {
    const std::string json = R"({
      "root": {"text": "A", "description": ""},
      "arguments": [
        {"target": "root", "kind": "supports", "basis": "", "claims": [{"id": "c1", "text": "B", "description": ""}]}
      ]
    })";

    const auto result = GeneratedDebateBuilder::Build(json, user_id_, *moderator_);
    ASSERT_TRUE(result.ok) << result.error;

    for (const auto& blob : wrapper_->getStatementsForDebate(result.debateId)) {
        debate::Claim claim;
        ASSERT_TRUE(claim.ParseFromArray(blob.data(), static_cast<int>(blob.size())));
        EXPECT_EQ(claim.creator_id(), user_id_);
    }
    EXPECT_GT(wrapper_->getMovesForDebate(result.debateId).size(), 0u)
        << "a generated debate must appear in the move log like any other";
}
