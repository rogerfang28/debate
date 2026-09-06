// test_connections_debate_builder.cc -- two phases, one debate.
//
// Phase 1 is a numbered list, phase 2 is one JSON object per line. The point
// of that split is that a mistake costs a LINE, not the run, so most of these
// tests feed in something partly broken and check that the good part still
// lands and the bad part is named.

#include <gtest/gtest.h>

#include "../../backend/src/utils/ConnectionsDebateBuilder.h"
#include "../../backend/src/utils/DebateGenerator.h"
#include "../../backend/src/utils/BasisRegistry.h"
#include "debate.pb.h"
#include "database/sqlite/Database.h"
#include "utils/DebateWrapper.h"
#include "debateModerator/DebateModerator.h"

#include <cstdio>
#include <map>
#include <string>
#include <vector>

namespace {

using Builder = ConnectionsDebateBuilder;

// ---------------------------------------------------------------------------
// Phase-1 parsing: any list marker, and the model's numbers are not trusted.
// ---------------------------------------------------------------------------
TEST(ParseClaims, StripsEveryKindOfListMarker) {
    const auto claims = Builder::ParseClaims(
        "1. Alpha\n2) Beta\n3: Gamma\n[4] Delta\n#5 Epsilon\n- Zeta\n* Eta\n\"Theta\"\n");
    const std::vector<std::string> expected = {"Alpha", "Beta", "Gamma", "Delta",
                                               "Epsilon", "Zeta", "Eta", "Theta"};
    EXPECT_EQ(claims, expected);
}

TEST(ParseClaims, IgnoresBlankLinesAndAFence) {
    const auto claims = Builder::ParseClaims("```\n1. One\n\n\n2. Two\n```\n");
    EXPECT_EQ(claims, (std::vector<std::string>{"One", "Two"}));
}

// The model's own numbering is discarded; ORDER is what numbers a claim. So a
// model that skips or repeats a number cannot shift anything.
TEST(ParseClaims, OrderNotTheModelsNumbersDecidesTheNumber) {
    const auto claims = Builder::ParseClaims("1. A\n1. B\n7. C\n");
    EXPECT_EQ(claims, (std::vector<std::string>{"A", "B", "C"}));
}

// ---------------------------------------------------------------------------
// Phase-2 parsing: line by line, tolerant of array habits, strict on content.
// ---------------------------------------------------------------------------
TEST(ParseConnections, OneObjectPerLine) {
    std::vector<std::string> dropped;
    const auto c = Builder::ParseConnections(
        R"({"from": [2], "to": 1, "role": "supports", "basis": "EVIDENTIAL", "why": "w"}
{"from": [3, 4], "to": 1, "role": "opposes", "basis": "NORMATIVE", "joint_reason": "j"})",
        dropped);
    ASSERT_EQ(c.size(), 2u);
    EXPECT_TRUE(dropped.empty());
    EXPECT_EQ(c[0].from, (std::vector<int>{2}));
    EXPECT_EQ(c[0].to, 1);
    EXPECT_FALSE(c[0].opposes);
    EXPECT_EQ(c[0].basis, "EVIDENTIAL");
    EXPECT_EQ(c[0].why, "w");
    EXPECT_EQ(c[1].from, (std::vector<int>{3, 4}));
    EXPECT_TRUE(c[1].opposes);
    EXPECT_EQ(c[1].joint_reason, "j");
}

TEST(ParseConnections, AcceptsAWholeArrayAndTrailingCommas) {
    std::vector<std::string> dropped;
    const auto asArray = Builder::ParseConnections(
        R"([{"from": [2], "to": 1, "role": "supports"}, {"from": [3], "to": 2, "role": "opposes"}])",
        dropped);
    EXPECT_EQ(asArray.size(), 2u);
    const auto withCommas = Builder::ParseConnections(
        "[\n{\"from\": [2], \"to\": 1, \"role\": \"supports\"},\n{\"from\": [3], \"to\": 2, \"role\": \"opposes\"},\n]",
        dropped);
    EXPECT_EQ(withCommas.size(), 2u);
    EXPECT_TRUE(dropped.empty());
}

TEST(ParseConnections, LenientOnRoleWordsAndABareFromNumber) {
    std::vector<std::string> dropped;
    const auto c = Builder::ParseConnections(
        R"({"from": 2, "to": 1, "role": "SUPPORTS"}
{"from": [3], "to": 1, "role": "challenges"}
{"from": [4], "to": 1, "role": "against"})",
        dropped);
    ASSERT_EQ(c.size(), 3u);
    EXPECT_FALSE(c[0].opposes);
    EXPECT_TRUE(c[1].opposes);
    EXPECT_TRUE(c[2].opposes);
}

// The whole reason for the format: one bad line, one dropped link, named.
TEST(ParseConnections, ABadLineCostsOneLinkAndSaysWhy) {
    std::vector<std::string> dropped;
    const auto c = Builder::ParseConnections(
        R"({"from": [2], "to": 1, "role": "supports"}
this is not json at all
{"from": [3], "to": 1, "role": "maybe"}
{"to": 1, "role": "supports"}
{"from": [4], "to": 1, "role": "opposes"})",
        dropped);
    EXPECT_EQ(c.size(), 2u) << "the two well-formed lines survive";
    ASSERT_EQ(dropped.size(), 3u);
    EXPECT_NE(dropped[0].find("not JSON"), std::string::npos);
    EXPECT_NE(dropped[1].find("role"), std::string::npos);
    EXPECT_NE(dropped[2].find("from"), std::string::npos);
}

// ---------------------------------------------------------------------------
// Building. A real moderator, so the move log is in the loop.
// ---------------------------------------------------------------------------
class ConnectionsBuildTest : public ::testing::Test {
protected:
    void SetUp() override {
        dbPath_ = "test_connections_" + std::to_string(getpid()) + ".sqlite3";
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

    Builder::Result build(const std::string& claims, const std::string& connections) {
        return Builder::Build(claims, connections, userId_, *moderator_);
    }

    // (from, to, basis, linkType, relationId)
    std::vector<std::tuple<int, int, std::string, int, int>> links(int debateId) {
        std::vector<std::tuple<int, int, std::string, int, int>> out;
        for (const auto& row : moderator_->getDebateWrapper().getLinksForDebate(debateId)) {
            out.emplace_back(std::get<1>(row), std::get<2>(row), std::get<3>(row),
                             std::get<5>(row), std::get<6>(row));
        }
        return out;
    }
    int idOf(int debateId, const std::string& needle) {
        for (const auto& blob : moderator_->getDebateWrapper().getStatementsForDebate(debateId)) {
            debate::Claim c;
            if (c.ParseFromArray(blob.data(), static_cast<int>(blob.size())) &&
                c.sentence().find(needle) != std::string::npos) {
                return c.id();
            }
        }
        return 0;
    }
    int debates() {
        return static_cast<int>(moderator_->getDebateWrapper().getUserDebateIds(userId_).size());
    }

    std::string dbPath_;
    Database* usersDb_ = nullptr;
    DebateModerator* moderator_ = nullptr;
    int userId_ = 0;
};

const char* kSpeedClaims =
    "1. Cities should lower urban speed limits to 20mph\n"
    "2. Crash deaths fell 23 percent in the trial boroughs\n"
    "3. The trial boroughs were already getting safer before the change\n"
    "4. Casualty rates in those boroughs matched the city average the year before\n"
    "5. Stopping distance at 20mph is about half what it is at 30mph\n"
    "6. A pedestrian struck at 20mph is five times more likely to survive\n"
    "7. The WHO recommends 30 km/h limits where people and vehicles mix\n";

// Depth comes from chaining. 4 attacks 3 attacks 2 supports 1: three deep,
// and the rebuttal lands on the objection, not the root.
TEST_F(ConnectionsBuildTest, ChainedConnectionsBecomeDepth) {
    const auto r = build(kSpeedClaims,
        R"({"from": [2], "to": 1, "role": "supports", "basis": "EVIDENTIAL", "why": "measured"}
{"from": [3], "to": 2, "role": "opposes", "basis": "INFERENTIAL", "why": "what it shows"}
{"from": [4], "to": 3, "role": "opposes", "basis": "EVIDENTIAL", "why": "a measurement"}
{"from": [5, 6], "to": 1, "role": "supports", "basis": "EVIDENTIAL", "joint_reason": "neither argues for the limit alone"}
{"from": [7], "to": 1, "role": "supports", "basis": "AUTHORITY", "why": "who decides"})");
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.claimsCreated, 7);
    EXPECT_EQ(r.argumentsCreated, 5);
    EXPECT_EQ(r.maxDepthReached, 3);
    EXPECT_TRUE(r.dropped.empty());
    EXPECT_TRUE(r.orphans.empty());
    EXPECT_EQ(r.groupsSplit, 0);

    const int objection = idOf(r.debateId, "already getting safer");
    const int rebuttal = idOf(r.debateId, "matched the city average");
    bool rebuttalOnObjection = false;
    std::map<int, int> perRelation;
    for (const auto& l : links(r.debateId)) {
        if (std::get<0>(l) == rebuttal && std::get<1>(l) == objection) rebuttalOnObjection = true;
        perRelation[std::get<4>(l)]++;
    }
    EXPECT_TRUE(rebuttalOnObjection);
    int joint = 0;
    for (const auto& e : perRelation) if (e.second == 2) joint++;
    EXPECT_EQ(joint, 1) << "5 and 6 share one relation";
}

TEST_F(ConnectionsBuildTest, AJointWithoutAReasonIsSplit) {
    const auto r = build(kSpeedClaims,
        R"({"from": [5, 6], "to": 1, "role": "supports", "basis": "EVIDENTIAL"})");
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.groupsSplit, 1);
    std::map<int, int> perRelation;
    for (const auto& l : links(r.debateId)) perRelation[std::get<4>(l)]++;
    EXPECT_EQ(perRelation.size(), 2u);
}

// Everything the model got wrong is dropped and NAMED; everything it got
// right is built. Claims nothing placed are reported as orphans, never guessed.
TEST_F(ConnectionsBuildTest, BadConnectionsAreDroppedByNameAndTheRestBuilt) {
    const auto r = build(kSpeedClaims,
        R"({"from": [2], "to": 1, "role": "supports", "basis": "EVIDENTIAL"}
{"from": [1], "to": 2, "role": "supports"}
{"from": [3], "to": 3, "role": "opposes"}
{"from": [4], "to": 99, "role": "opposes"}
{"from": [42], "to": 1, "role": "supports"}
{"from": [2], "to": 7, "role": "supports"}
{"from": [5], "to": 6, "role": "supports"}
{"from": [6], "to": 5, "role": "supports"})");
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.claimsCreated, 2) << "root plus the one good connection";
    // root as from, self-link, target 99, claim 42, claim 2 placed twice,
    // and the 5<->6 cycle (two lines that never find a created target).
    EXPECT_EQ(r.dropped.size(), 7u);
    auto has = [&](const char* s) {
        for (const auto& d : r.dropped) if (d.find(s) != std::string::npos) return true;
        return false;
    };
    EXPECT_TRUE(has("root"));
    EXPECT_TRUE(has("to itself"));
    EXPECT_TRUE(has("claim 99"));
    EXPECT_TRUE(has("claim 42"));
    EXPECT_TRUE(has("already placed"));
    EXPECT_TRUE(has("never created"));
    // 3, 4, 5, 6, 7 never got a usable placement.
    EXPECT_EQ(r.orphans, (std::vector<int>{3, 4, 5, 6, 7}));
}

// The failure a live run actually produced: the model connected 3 and 7 to
// claim 2 and never said where 2 itself goes. Without this, 2 and everything
// under it vanished. With it, 2 hangs off the root -- labelled as the
// builder's placement -- and its subtree survives.
TEST_F(ConnectionsBuildTest, AHubTheModelForgotIsAttachedToTheRootNotLost) {
    const auto r = build(kSpeedClaims,
        R"({"from": [3], "to": 2, "role": "opposes", "basis": "INFERENTIAL"}
{"from": [4], "to": 3, "role": "opposes", "basis": "EVIDENTIAL"}
{"from": [7], "to": 1, "role": "supports", "basis": "AUTHORITY"})");
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.attachedToRoot, 1) << "claim 2 was targeted but never placed";
    EXPECT_EQ(r.claimsCreated, 5) << "root, 2, 3, 4, 7";
    EXPECT_TRUE(r.dropped.empty()) << "nothing was lost to the omission";
    EXPECT_EQ(r.orphans, (std::vector<int>{5, 6})) << "true leaves nobody placed stay orphans";
    EXPECT_EQ(r.maxDepthReached, 3) << "the rescued subtree keeps its depth";

    const int root = idOf(r.debateId, "20mph");
    const int hub = idOf(r.debateId, "Crash deaths fell");
    // A SUPPORT link is stored parent -> child (the legacy PARENT_CHILD shape);
    // only CHALLENGE links run challenger -> challenged. Easy to get backwards,
    // and the two other tests here only ever checked challenges.
    bool hubUnderRoot = false;
    for (const auto& l : links(r.debateId)) {
        if (std::get<0>(l) == root && std::get<1>(l) == hub) {
            hubUnderRoot = true;
            EXPECT_TRUE(std::get<2>(l).empty() || std::get<2>(l) == "parent to child connection")
                << "a guessed placement carries no basis";
        }
    }
    EXPECT_TRUE(hubUnderRoot);
}

TEST_F(ConnectionsBuildTest, UnknownBasisIsDroppedButTheLinkSurvives) {
    const auto r = build(kSpeedClaims,
        R"({"from": [2], "to": 1, "role": "supports", "basis": "VIBES"})");
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.unknownBases, 1);
    EXPECT_EQ(r.claimsCreated, 2);
    for (const auto& l : links(r.debateId)) EXPECT_NE(std::get<2>(l), "VIBES");
}

TEST_F(ConnectionsBuildTest, NoUsableConnectionsMeansNoDebate) {
    const int before = debates();
    const auto r = build(kSpeedClaims, "complete garbage\nmore garbage\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.error.find("none of its connections"), std::string::npos);
    EXPECT_EQ(debates(), before) << "nothing half-built left behind";
}

TEST_F(ConnectionsBuildTest, NoClaimsMeansNoDebate) {
    const int before = debates();
    const auto r = build("", R"({"from": [2], "to": 1, "role": "supports"})");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(debates(), before);
}

TEST_F(ConnectionsBuildTest, ASingleClaimIsARootOnlyDebate) {
    const auto r = build("1. Just a root\n", "");
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_EQ(r.claimsCreated, 1);
}

TEST_F(ConnectionsBuildTest, TheBuildReachesTheMoveLog) {
    const auto r = build(kSpeedClaims,
        R"({"from": [2], "to": 1, "role": "supports", "basis": "EVIDENTIAL"})");
    ASSERT_TRUE(r.ok) << r.error;
    EXPECT_GE(moderator_->getDebateWrapper().getMovesForDebate(r.debateId).size(), 2u);
}

// ---------------------------------------------------------------------------
// The two prompts carry what they must and no unfilled placeholders.
// ---------------------------------------------------------------------------
TEST(GeneratorPrompts, ClaimsPromptHasTopicAndPassageOnly) {
    const std::string p = DebateGenerator::BuildClaimsPrompt("A topic?", "The passage text.");
    EXPECT_NE(p.find("A topic?"), std::string::npos);
    EXPECT_NE(p.find("The passage text."), std::string::npos);
    EXPECT_EQ(p.find("{topic}"), std::string::npos);
    EXPECT_EQ(p.find("{passage}"), std::string::npos);
    // Reading only: no basis vocabulary here.
    EXPECT_EQ(p.find("{bases}"), std::string::npos);
}

TEST(GeneratorPrompts, ConnectionsPromptNumbersTheClaimsAndCarriesTheRegistry) {
    const std::string p =
        DebateGenerator::BuildConnectionsPrompt("A topic", {"Root claim", "Second", "Third"});
    EXPECT_NE(p.find("1. Root claim\n2. Second\n3. Third\n"), std::string::npos);
    for (const auto& name : bases::Names()) {
        EXPECT_NE(p.find(name), std::string::npos) << name;
    }
    for (const char* key : {"{topic}", "{claims}", "{bases}", "{default_basis}"}) {
        EXPECT_EQ(p.find(key), std::string::npos) << key;
    }
}

} // namespace
