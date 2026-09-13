// test_basis_registry.cc -- the link kinds come from a file, and the file
// is obeyed.
//
// Two groups. The first drives bases::* against scratch registry files and
// checks it reads, validates, reloads and explains them. The second runs the
// REAL status engine on a real debate twice -- once under the shipped registry
// and once under a scratch one that changes a single propagation value -- and
// checks the concession that sinks the root under COLLAPSE no longer does
// under UNSUPPORT. That second test is the reason the registry exists.

#include <gtest/gtest.h>

#include "../../backend/src/utils/BasisRegistry.h"
#include "basis.pb.h"
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
#include "../../backend/src/utils/DebateGenerator.h"
#include "../../backend/src/utils/AiChallenger.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

namespace {

void setEnv(const char* name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    if (value.empty()) unsetenv(name); else setenv(name, value.c_str(), 1);
#endif
}

std::filesystem::path repoRoot() {
    // test/tests/this_file -> test/tests -> test -> repo
    return std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
}

std::filesystem::path shippedRegistry() {
    return repoRoot() / "backend" / "config" / "bases.pbtxt";
}

const char* kFourBases = R"(
bases { name: "EVIDENTIAL"   propagation: COLLAPSE is_default: true description: "facts" as_support: "observed" as_challenge: "not shown" }
bases { name: "AUTHORITY"    propagation: COLLAPSE description: "who says so" as_support: "decided" as_challenge: "no standing" }
bases { name: "DEFINITIONAL" propagation: COLLAPSE description: "what a term means" as_support: "covered" as_challenge: "two senses" }
bases { name: "NORMATIVE"    propagation: COLLAPSE description: "values" as_support: "follows" as_challenge: "value as fact" }
)";

// Points the registry at a scratch file for the duration of one test and puts
// everything back afterwards, so no other suite inherits a deleted path.
class RegistryFileTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = std::filesystem::temp_directory_path() /
               ("bases_test_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
        std::filesystem::remove_all(dir_);
        std::filesystem::create_directories(dir_);
        file_ = dir_ / "bases.pbtxt";
        use(file_);
    }
    void TearDown() override {
        setEnv("DEBATE_BASES_FILE", "");
        bases::ResetCache();
        std::filesystem::remove_all(dir_);
    }

    void use(const std::filesystem::path& p) {
        setEnv("DEBATE_BASES_FILE", p.string());
        bases::ResetCache();
    }
    void write(const std::string& contents) {
        // Nudge the clock so a rewrite is never within timestamp granularity of
        // the previous one -- the reload check compares mtime and size.
        std::this_thread::sleep_for(std::chrono::milliseconds(15));
        std::ofstream(file_) << contents;
    }

    std::filesystem::path dir_;
    std::filesystem::path file_;
};

TEST_F(RegistryFileTest, ReadsTheShippedRegistry) {
    use(shippedRegistry());
    // Five, and no more than five: the set is the minimum that does not bend
    // an argument to fit it. A change here should come with the failure mode
    // the new entry has that none of the others do.
    const std::vector<std::string> expected = {"EVIDENTIAL", "AUTHORITY", "DEFINITIONAL",
                                               "NORMATIVE", "INFERENTIAL"};
    EXPECT_EQ(bases::Names(), expected);
    EXPECT_EQ(bases::DefaultName(), "EVIDENTIAL");
    for (const auto& name : expected) {
        EXPECT_TRUE(bases::IsKnown(name)) << name;
    }
    // The original four keep the rule every link had before the registry
    // existed. INFERENTIAL is the one shipped basis that does not collapse:
    // a bad step leaves its conclusion unproven, not false.
    for (const auto& name : {"EVIDENTIAL", "AUTHORITY", "DEFINITIONAL", "NORMATIVE"}) {
        EXPECT_EQ(bases::PropagationFor(name), basis::COLLAPSE) << name;
    }
    EXPECT_EQ(bases::PropagationFor("INFERENTIAL"), basis::UNSUPPORT);
}

TEST_F(RegistryFileTest, ExplainsItselfToAModelInItsOwnWords) {
    use(shippedRegistry());
    const std::string list = bases::GuideAsList();
    const std::string inline_ = bases::GuideInline();
    for (const auto& name : bases::Names()) {
        EXPECT_NE(list.find(name), std::string::npos) << name;
        EXPECT_NE(inline_.find(name), std::string::npos) << name;
    }
    // Both halves of the guide reach the list form; the inline form is the
    // challenger's and carries the challenge gloss.
    EXPECT_NE(list.find("Supporting:"), std::string::npos);
    EXPECT_NE(list.find("Challenging:"), std::string::npos);
    EXPECT_NE(inline_.find("motive"), std::string::npos)
        << "AUTHORITY's challenge gloss should mention motive";
}

TEST_F(RegistryFileTest, OnlyRegistryNamesSurviveTheWritePath) {
    write(kFourBases);
    EXPECT_EQ(bases::Sanitize("AUTHORITY"), "AUTHORITY");
    EXPECT_EQ(bases::Sanitize(""), "") << "unspecified is allowed";
    EXPECT_EQ(bases::Sanitize("VIBES"), "") << "unknown is dropped, not stored";
    EXPECT_EQ(bases::Sanitize("evidential"), "") << "names are exact";
}

TEST_F(RegistryFileTest, LinksFromBeforeBasesExistedStillCollapse) {
    write(kFourBases);
    EXPECT_EQ(bases::PropagationFor(""), basis::COLLAPSE);
    EXPECT_EQ(bases::PropagationFor("parent to child connection"), basis::COLLAPSE);
    EXPECT_EQ(bases::PropagationFor("challenge link"), basis::COLLAPSE);
    EXPECT_EQ(bases::PropagationFor("NOT_A_BASIS"), basis::COLLAPSE);
}

// The point of the file: a new kind of reason is a block of text, and every
// reader -- dropdown, prompt, engine -- sees it with no code change.
TEST_F(RegistryFileTest, AFifthBasisIsAFileEdit) {
    write(std::string(kFourBases) +
          R"(bases { name: "PRECEDENT" propagation: UNSUPPORT description: "a prior ruling" as_support: "was decided this way before" as_challenge: "the earlier case differs" })");
    ASSERT_EQ(bases::Names().size(), 5u);
    EXPECT_EQ(bases::Names().back(), "PRECEDENT");
    EXPECT_TRUE(bases::IsKnown("PRECEDENT"));
    EXPECT_EQ(bases::Sanitize("PRECEDENT"), "PRECEDENT");
    EXPECT_EQ(bases::PropagationFor("PRECEDENT"), basis::UNSUPPORT);
    EXPECT_NE(bases::GuideAsList().find("prior ruling"), std::string::npos);
    EXPECT_NE(bases::GuideInline().find("earlier case differs"), std::string::npos);
}

TEST_F(RegistryFileTest, ReloadsWhenTheFileChangesWithoutAReset) {
    write(R"(bases { name: "ONE" } bases { name: "TWO" })");
    ASSERT_EQ(bases::Names().size(), 2u);
    write(R"(bases { name: "ONE" } bases { name: "TWO" } bases { name: "THREE" })");
    EXPECT_EQ(bases::Names().size(), 3u) << "an edit should show up on the next access";
}

TEST_F(RegistryFileTest, AnUnparseableEditKeepsWhatWasLoaded) {
    write(R"(bases { name: "ONE" } bases { name: "TWO" })");
    ASSERT_EQ(bases::Names().size(), 2u);
    write("bases { name: \"ONE\" this is not text format");
    // A half-saved edit must not blank a running server's dropdowns.
    EXPECT_EQ(bases::Names().size(), 2u);
}

TEST_F(RegistryFileTest, AMissingFileIsSurvivable) {
    use(dir_ / "nowhere.pbtxt");
    EXPECT_TRUE(bases::Names().empty());
    EXPECT_EQ(bases::DefaultName(), "");
    EXPECT_EQ(bases::Sanitize("EVIDENTIAL"), "") << "nothing is known when nothing is loaded";
    EXPECT_EQ(bases::PropagationFor("EVIDENTIAL"), basis::COLLAPSE)
        << "the fallback is the behaviour links have always had";
    EXPECT_FALSE(bases::GuideAsList().empty()) << "a prompt should say so, not omit the section";
}

TEST_F(RegistryFileTest, DefaultIsTheFlaggedEntryElseTheFirst) {
    write(R"(bases { name: "A" } bases { name: "B" is_default: true } bases { name: "C" })");
    EXPECT_EQ(bases::DefaultName(), "B");
    write(R"(bases { name: "A" } bases { name: "B" } bases { name: "C" })");
    EXPECT_EQ(bases::DefaultName(), "A");
}

// Both prompts pull the kinds from the registry at build time. A placeholder
// left unsubstituted would reach the model as literal "{bases}", and the model
// would do something with it -- so the check is for the names AND for the
// absence of the keys.
TEST_F(RegistryFileTest, BothPromptsCarryTheRegistryAndNoPlaceholders) {
    use(shippedRegistry());
    const std::string gen = DebateGenerator::BuildPrompt("a topic", "a passage");
    const std::string chal = AiChallenger::BuildPrompt("[#1] a claim");
    for (const auto& name : bases::Names()) {
        EXPECT_NE(gen.find(name), std::string::npos) << "generator lacks " << name;
        EXPECT_NE(chal.find(name), std::string::npos) << "challenger lacks " << name;
    }
    for (const char* key : {"{bases}", "{default_basis}"}) {
        EXPECT_EQ(gen.find(key), std::string::npos) << key;
        EXPECT_EQ(chal.find(key), std::string::npos) << key;
    }
    // The default is what the examples use, so a reader of the prompt sees a
    // real basis name where the template said {default_basis}.
    EXPECT_NE(gen.find("basis: \"EVIDENTIAL\""), std::string::npos);
}

} // namespace


// ---------------------------------------------------------------------------
// The REPLAY engine obeys the registry too. Same shape as the stored-status
// pair above, but nothing is stored: a graph and a move log go in, statuses
// come out. This is the pair that has to hold once the stored path is gone.
// ---------------------------------------------------------------------------
#include "statusEngine/StatusEngine.h"

namespace {

debate::Move replayMove(int seq, int actor, debate::MoveType type, int claimId,
                        const std::string& payload = "") {
    debate::Move m;
    m.set_seq(seq);
    m.set_actor_id(actor);
    m.set_type(type);
    m.set_target_type(debate::TARGET_CLAIM);
    m.set_target_id(claimId);
    m.set_payload(payload);
    return m;
}

debate::Relationship replayRelation(int id, int from, int to, debate::RelationRole role,
                                    const std::string& basisName) {
    debate::Relationship r;
    r.set_id(id);
    r.add_from_claim_ids(from);
    r.set_to_claim_id(to);
    r.set_role(role);
    r.set_basis(basisName);
    return r;
}

debate::ClaimStatus replayed(const StatusEngine::LegacyResult& r, const std::string& user,
                             int claimId) {
    auto u = r.by_user.find(user);
    if (u == r.by_user.end()) return debate::ClaimStatus::UNDETERMINED;
    auto c = u->second.find(claimId);
    return c == u->second.end() ? debate::ClaimStatus::UNDETERMINED : c->second;
}

// Alice asserts a root with one EVIDENTIAL reason; Bob challenges the reason;
// Alice concedes. Returns the root as Alice sees it, replayed from the log.
debate::ClaimStatus replayedRootForAliceAfterConceding() {
    std::vector<debate::Claim> claims;
    for (int id : {1, 2, 3}) { debate::Claim c; c.set_id(id); claims.push_back(c); }
    const std::vector<debate::Relationship> relations{
        replayRelation(10, 2, 1, debate::SUPPORTS, "EVIDENTIAL"),
        replayRelation(11, 3, 2, debate::OPPOSES, "EVIDENTIAL"),
    };
    const std::vector<debate::Move> moves{
        replayMove(1, 1, debate::ASSERT, 1),
        replayMove(2, 1, debate::ASSERT, 2),
        replayMove(3, 2, debate::ASSERT, 3),
        replayMove(4, 2, debate::OPPOSE, 2, "{\"relation_id\":11,\"challenging_claim_id\":3}"),
        replayMove(5, 1, debate::CONCEDE, 2, "{\"challenge_relation_id\":11,\"challenging_claim_id\":3}"),
    };
    const auto r = StatusEngine::computeLegacyStatuses(
        claims, relations, moves, {{1, "alice"}, {2, "bob"}});
    EXPECT_EQ(replayed(r, "alice", 2), debate::ClaimStatus::FALSE_CLAIM)
        << "the concession itself, under any registry";
    return replayed(r, "alice", 1);
}

}  // namespace

TEST_F(RegistryFileTest, ReplayUnderCollapseAConcededReasonSinksTheRoot) {
    use(shippedRegistry());
    EXPECT_EQ(replayedRootForAliceAfterConceding(), debate::ClaimStatus::FALSE_CLAIM);
}

TEST_F(RegistryFileTest, ReplayUnderUnsupportAConcededReasonLeavesTheRootStanding) {
    write(R"(
bases { name: "EVIDENTIAL" propagation: UNSUPPORT is_default: true }
)");
    // The reason is gone, not refuted: Alice's own assertion of the root is
    // all that is left, and that is TRUE, not FALSE.
    EXPECT_EQ(replayedRootForAliceAfterConceding(), debate::ClaimStatus::TRUE_CLAIM);
}

// A challenge held TRUE on an UNSUPPORT edge takes the claim's TRUE away
// without making it FALSE; on a COLLAPSE edge it refutes. Carol holds both
// the root and Bob's challenge, so Rule D is what decides her view.
TEST_F(RegistryFileTest, ReplayRuleDUnsupportDemotesToUndeterminedNotFalse) {
    auto run = [&](const std::string& basisName) {
        std::vector<debate::Claim> claims;
        for (int id : {1, 3}) { debate::Claim c; c.set_id(id); claims.push_back(c); }
        const std::vector<debate::Relationship> relations{
            replayRelation(11, 3, 1, debate::OPPOSES, basisName),
        };
        const std::vector<debate::Move> moves{
            replayMove(1, 1, debate::ASSERT, 1),
            replayMove(2, 2, debate::ASSERT, 3),
            replayMove(3, 2, debate::OPPOSE, 1, "{\"relation_id\":11,\"challenging_claim_id\":3}"),
            replayMove(4, 3, debate::ACCEPT, 1),
            replayMove(5, 3, debate::ACCEPT, 3),
        };
        const auto r = StatusEngine::computeLegacyStatuses(
            claims, relations, moves, {{1, "alice"}, {2, "bob"}, {3, "carol"}});
        return replayed(r, "carol", 1);
    };
    use(shippedRegistry());
    EXPECT_EQ(run("EVIDENTIAL"), debate::ClaimStatus::FALSE_CLAIM) << "COLLAPSE refutes";
    EXPECT_EQ(run("INFERENTIAL"), debate::ClaimStatus::UNDETERMINED) << "UNSUPPORT only takes the TRUE away";
}

// A NONE edge carries nothing in either direction.
TEST_F(RegistryFileTest, ReplayANoneEdgeCarriesNothing) {
    write(R"(
bases { name: "EVIDENTIAL" propagation: COLLAPSE is_default: true }
bases { name: "CONTAINS"   propagation: NONE }
)");
    std::vector<debate::Claim> claims;
    for (int id : {1, 2}) { debate::Claim c; c.set_id(id); claims.push_back(c); }
    const std::vector<debate::Relationship> relations{
        replayRelation(10, 2, 1, debate::SUPPORTS, "CONTAINS"),
    };
    // Bob holds the child FALSE; under COLLAPSE that would sink the root for
    // him. Under NONE the root is untouched -- he has no view on it at all.
    const std::vector<debate::Move> moves{
        replayMove(1, 1, debate::ASSERT, 1),
        replayMove(2, 1, debate::ASSERT, 2),
        replayMove(3, 2, debate::ASSERT, 3),
        replayMove(4, 2, debate::OPPOSE, 2, "{\"relation_id\":11,\"challenging_claim_id\":3}"),
    };
    const auto r = StatusEngine::computeLegacyStatuses(
        claims, relations, moves, {{1, "alice"}, {2, "bob"}});
    EXPECT_EQ(replayed(r, "bob", 2), debate::ClaimStatus::FALSE_CLAIM);
    EXPECT_EQ(replayed(r, "bob", 1), debate::ClaimStatus::UNDETERMINED);
    EXPECT_EQ(replayed(r, "alice", 1), debate::ClaimStatus::TRUE_CLAIM);
}
