// test_extracted_debate_builder.cc -- a nested tree becomes a real debate.
//
// The tree format exists because the step list it replaces made the model
// number its own claims: a captured run sent all nine navigations to claim 1
// and produced no joint arguments, because the root is the only id that is
// always safe and a wrong id failed silently. So the cases that matter most
// here are the ones the old format got wrong -- depth, and joint vs
// independent -- plus proof that a malformed tree is refused LOUDLY rather
// than half-imported.

#include <gtest/gtest.h>

#include "../../backend/src/utils/ExtractedDebateBuilder.h"
#include "extraction.pb.h"
#include "debate.pb.h"
#include "user.pb.h"
#include "user_engagement.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "utils/DebateWrapper.h"
#include "debateModerator/DebateModerator.h"

#include <cstdio>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace {

class ExtractedDebateBuilderTest : public ::testing::Test {
protected:
    void SetUp() override {
        dbPath_ = "test_extracted_" + std::to_string(getpid()) + ".sqlite3";
        std::remove(dbPath_.c_str());
#ifdef _WIN32
        _putenv_s("DB_PATH", dbPath_.c_str());
#else
        setenv("DB_PATH", dbPath_.c_str(), 1);
#endif
        usersDb_ = new Database(dbPath_);
        moderator_ = new DebateModerator(*usersDb_);
        userId_ = moderator_->createUserIfNotExist("author");
    }

    void TearDown() override {
        delete moderator_;
        delete usersDb_;
#ifdef _WIN32
        _putenv_s("DB_PATH", "");
#else
        unsetenv("DB_PATH");
#endif
        std::remove(dbPath_.c_str());
        std::remove((dbPath_ + "-wal").c_str());
        std::remove((dbPath_ + "-shm").c_str());
    }

    ExtractedDebateBuilder::Result build(const std::string& tree) {
        return ExtractedDebateBuilder::Build(tree, userId_, *moderator_);
    }

    // The debate's claims by id, so a test can assert on the shape that landed.
    std::map<int, debate::Claim> claimsOf(int debateId) {
        std::map<int, debate::Claim> out;
        for (const auto& blob : moderator_->getDebateWrapper().getStatementsForDebate(debateId)) {
            debate::Claim c;
            if (c.ParseFromArray(blob.data(), static_cast<int>(blob.size()))) {
                out[c.id()] = c;
            }
        }
        return out;
    }

    // (from, to, basis, linkType, relationId) for every link in the debate.
    std::vector<std::tuple<int, int, std::string, int, int>> linksOf(int debateId) {
        std::vector<std::tuple<int, int, std::string, int, int>> out;
        for (const auto& row : moderator_->getDebateWrapper().getLinksForDebate(debateId)) {
            out.emplace_back(std::get<1>(row), std::get<2>(row), std::get<3>(row),
                             std::get<5>(row), std::get<6>(row));
        }
        return out;
    }

    int claimIdByText(int debateId, const std::string& needle) {
        for (const auto& entry : claimsOf(debateId)) {
            if (entry.second.sentence().find(needle) != std::string::npos) {
                return entry.first;
            }
        }
        return 0;
    }

    std::string dbPath_;
    Database* usersDb_ = nullptr;
    DebateModerator* moderator_ = nullptr;
    int userId_ = 0;
};

// ---------------------------------------------------------------------------
// Depth. The old format's central failure: everything landed on the root.
// Here depth is just nesting, so a four-level tree arrives four levels deep.
// ---------------------------------------------------------------------------
TEST_F(ExtractedDebateBuilderTest, NestingProducesDepthWithNoIdsAnywhere) {
    const auto result = build(R"(
root {
  text: "Cities should lower urban speed limits to 20mph"
  arguments {
    role: SUPPORTS
    basis: "EVIDENTIAL"
    why: "rests on what was measured"
    claims {
      text: "Crash deaths fell 23 percent in the trial zones"
      arguments {
        role: OPPOSES
        basis: "INFERENTIAL"
        why: "the figure is real; what it shows is the question"
        claims {
          text: "The trial zones were already safer than average"
          arguments {
            role: OPPOSES
            basis: "EVIDENTIAL"
            why: "a measurement against that objection"
            claims { text: "The zones matched the city average before the trial" }
          }
        }
      }
    }
  }
}
)");
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.claimsCreated, 4);
    EXPECT_EQ(result.argumentsCreated, 3);
    // Root at 0, so a chain of three arguments reaches depth 3.
    EXPECT_EQ(result.maxDepthReached, 3);

    // The rebuttal hangs off the objection, NOT off the root -- the thing the
    // step-list format could never reliably do.
    const int root = claimIdByText(result.debateId, "20mph");
    const int objection = claimIdByText(result.debateId, "already safer");
    const int rebuttal = claimIdByText(result.debateId, "matched the city average");
    ASSERT_GT(objection, 0);
    ASSERT_GT(rebuttal, 0);
    bool rebuttalUnderObjection = false;
    bool rebuttalUnderRoot = false;
    for (const auto& link : linksOf(result.debateId)) {
        if (std::get<0>(link) == rebuttal && std::get<1>(link) == objection) rebuttalUnderObjection = true;
        if (std::get<0>(link) == rebuttal && std::get<1>(link) == root) rebuttalUnderRoot = true;
    }
    EXPECT_TRUE(rebuttalUnderObjection);
    EXPECT_FALSE(rebuttalUnderRoot);
}

// ---------------------------------------------------------------------------
// Joint vs independent is a nesting choice, and both land correctly.
// ---------------------------------------------------------------------------
TEST_F(ExtractedDebateBuilderTest, OneBlockIsJointAndSeparateBlocksAreIndependent) {
    const auto result = build(R"(
root {
  text: "Fauci's testimony was false"
  arguments {
    role: SUPPORTS
    basis: "EVIDENTIAL"
    joint_reason: "the grant and the denial prove nothing without the definition"
    claims { text: "NIAID funded the Wuhan sub-award" }
    claims { text: "He denied funding that kind of work" }
    claims { text: "That work meets the definition" }
  }
  arguments {
    role: SUPPORTS
    basis: "EVIDENTIAL"
    why: "a separate route to the same charge"
    claims { text: "The October 2021 letter confirms the work enhanced a virus" }
  }
}
)");
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.claimsCreated, 5);
    EXPECT_EQ(result.groupsSplit, 0) << "a stated joint_reason should keep the group";

    // Three claims sharing ONE relation id, and the fourth on its own.
    std::map<int, int> membersPerRelation;
    for (const auto& link : linksOf(result.debateId)) {
        membersPerRelation[std::get<4>(link)]++;
    }
    std::multiset<int> sizes;
    for (const auto& entry : membersPerRelation) sizes.insert(entry.second);
    EXPECT_EQ(sizes, (std::multiset<int>{1, 3}))
        << "expected one joint relation of three and one independent of one";
}

// The model groups by default and groups wrongly, so a group with nothing
// stated is split rather than trusted.
TEST_F(ExtractedDebateBuilderTest, AGroupWithNoStatedReasonIsSplit) {
    const auto result = build(R"(
root {
  text: "The four-day week worked"
  arguments {
    role: SUPPORTS
    basis: "EVIDENTIAL"
    claims { text: "Revenue rose 1.4 percent" }
    claims { text: "Sick days fell by two thirds" }
    claims { text: "Turnover fell 57 percent" }
  }
}
)");
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.groupsSplit, 1);
    EXPECT_EQ(result.claimsCreated, 4);

    std::map<int, int> membersPerRelation;
    for (const auto& link : linksOf(result.debateId)) {
        membersPerRelation[std::get<4>(link)]++;
    }
    EXPECT_EQ(membersPerRelation.size(), 3u) << "three independent relations, not one joint";
    for (const auto& entry : membersPerRelation) {
        EXPECT_EQ(entry.second, 1);
    }
}

// ---------------------------------------------------------------------------
// Challenges are ordinary nodes: they carry a CHALLENGE link and can be
// argued with in turn.
// ---------------------------------------------------------------------------
TEST_F(ExtractedDebateBuilderTest, OpposesBecomesAChallengeLink) {
    const auto result = build(R"(
root {
  text: "Remote work raises productivity"
  arguments {
    role: OPPOSES
    basis: "AUTHORITY"
    why: "attacks who is measuring, not what was measured"
    claims { text: "The survey was run by a company selling remote-work software" }
  }
}
)");
    ASSERT_TRUE(result.ok) << result.error;
    bool sawChallenge = false;
    for (const auto& link : linksOf(result.debateId)) {
        if (std::get<3>(link) == static_cast<int>(debate::LinkType::CHALLENGE)) {
            sawChallenge = true;
            EXPECT_EQ(std::get<2>(link), "AUTHORITY") << "the basis rides on the link";
        }
    }
    EXPECT_TRUE(sawChallenge);
}

// ---------------------------------------------------------------------------
// The basis registry governs what can be stored, here as everywhere else.
// ---------------------------------------------------------------------------
TEST_F(ExtractedDebateBuilderTest, AnUnknownBasisIsDroppedButTheClaimSurvives) {
    const auto result = build(R"(
root {
  text: "A claim"
  arguments {
    role: SUPPORTS
    basis: "VIBES"
    claims { text: "A reason" }
  }
}
)");
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.unknownBases, 1);
    EXPECT_EQ(result.claimsCreated, 2) << "the claim is still created";
    for (const auto& link : linksOf(result.debateId)) {
        EXPECT_NE(std::get<2>(link), "VIBES");
    }
}

// ---------------------------------------------------------------------------
// Bad input is REFUSED, and refused before anything is written. This is the
// property the step list lacked: a mis-numbered step list still parsed and
// still built something, just wrong.
// ---------------------------------------------------------------------------
TEST_F(ExtractedDebateBuilderTest, MalformedInputIsRefusedWithNothingCreated) {
    const int debatesBefore =
        static_cast<int>(moderator_->getDebateWrapper().getUserDebateIds(userId_).size());

    struct Case { const char* name; const char* text; const char* expect; };
    const Case cases[] = {
        {"not text format", "root { text: \"x\" this is not protobuf", "not valid protobuf"},
        {"no root", "", "no root claim"},
        {"blank root text", "root { text: \"\" }", "no root claim"},
        {"argument with no role",
         "root { text: \"x\" arguments { basis: \"EVIDENTIAL\" claims { text: \"y\" } } }",
         "no role"},
        {"argument with no claims",
         "root { text: \"x\" arguments { role: SUPPORTS basis: \"EVIDENTIAL\" } }",
         "no claims"},
        {"child with no text",
         "root { text: \"x\" arguments { role: SUPPORTS claims { text: \"\" } } }",
         "no claims"},
    };
    for (const Case& c : cases) {
        const auto result = build(c.text);
        EXPECT_FALSE(result.ok) << c.name << " should have been refused";
        EXPECT_NE(result.error.find(c.expect), std::string::npos)
            << c.name << " gave: " << result.error;
        EXPECT_EQ(result.debateId, 0) << c.name << " created a debate anyway";
    }

    EXPECT_EQ(static_cast<int>(moderator_->getDebateWrapper().getUserDebateIds(userId_).size()),
              debatesBefore)
        << "a refused tree must leave no debate behind";
}

TEST_F(ExtractedDebateBuilderTest, ACodeFenceIsTolerated) {
    const auto result = build("```\nroot { text: \"A claim\" }\n```");
    ASSERT_TRUE(result.ok) << result.error;
    EXPECT_EQ(result.claimsCreated, 1);
}

// A debate is only real if it reached the move log -- statuses are computed
// from it, so a debate built around it would look right and be inert.
TEST_F(ExtractedDebateBuilderTest, TheBuildReachesTheMoveLog) {
    const auto result = build(R"(
root {
  text: "A claim"
  arguments { role: SUPPORTS basis: "EVIDENTIAL" claims { text: "A reason" } }
}
)");
    ASSERT_TRUE(result.ok) << result.error;
    const auto moves = moderator_->getDebateWrapper().getMovesForDebate(result.debateId);
    EXPECT_GE(moves.size(), 2u) << "creating and supporting a claim should both be logged";
}

} // namespace
