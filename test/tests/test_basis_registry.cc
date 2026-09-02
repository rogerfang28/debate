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
    const std::vector<std::string> expected = {"EVIDENTIAL", "AUTHORITY",  "DEFINITIONAL",
                                               "NORMATIVE",  "CAUSAL",     "ANALOGICAL",
                                               "INFERENTIAL"};
    EXPECT_EQ(bases::Names(), expected);
    EXPECT_EQ(bases::DefaultName(), "EVIDENTIAL");
    for (const auto& name : expected) {
        EXPECT_TRUE(bases::IsKnown(name)) << name;
    }
    // The original four keep the rule every link had before the registry
    // existed. INFERENTIAL is the one shipped basis that does not collapse:
    // a bad step leaves its conclusion unproven, not false.
    for (const auto& name : {"EVIDENTIAL", "AUTHORITY", "DEFINITIONAL", "NORMATIVE",
                             "CAUSAL", "ANALOGICAL"}) {
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

// ---------------------------------------------------------------------------
// The engine obeys the file.
//
// Same debate, same moves, two registries. Alice roots a debate, supports it
// with one EVIDENTIAL claim, Bob challenges that claim, Alice concedes.
//
//   COLLAPSE  (shipped)  -> the root is FALSE for Alice. This is the rule that
//                           sank the whole Fauci debate when one strand was
//                           conceded.
//   UNSUPPORT (scratch)  -> the root is NOT false for Alice: she lost her
//                           reason, she was not shown wrong.
// ---------------------------------------------------------------------------
class PropagationObeysRegistryTest : public RegistryFileTest {
protected:
    void SetUp() override {
        RegistryFileTest::SetUp();
        dbPath_ = "test_bases_prop_" + std::to_string(getpid()) + ".sqlite3";
        std::remove(dbPath_.c_str());
        db_ = new Database(dbPath_);
        wrapper_ = new DatabaseWrapper(*db_, *db_);
        debate_ = new DebateWrapper(*wrapper_);
        ASSERT_TRUE(wrapper_->ensureAllTables());
        alice_ = createUser("alice");
        bob_ = createUser("bob");
    }
    void TearDown() override {
        delete debate_;
        delete wrapper_;
        delete db_;
        std::remove(dbPath_.c_str());
        std::remove((dbPath_ + "-wal").c_str());
        std::remove((dbPath_ + "-shm").c_str());
        RegistryFileTest::TearDown();
    }

    int createUser(const std::string& name) {
        user::User u;
        u.set_username(name);
        u.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
        std::vector<uint8_t> bytes(u.ByteSizeLong());
        u.SerializeToArray(bytes.data(), static_cast<int>(bytes.size()));
        const int id = wrapper_->users.createUser(name, bytes);
        u.set_user_id(id);
        bytes.assign(u.ByteSizeLong(), 0);
        u.SerializeToArray(bytes.data(), static_cast<int>(bytes.size()));
        wrapper_->users.updateUserProtobuf(id, bytes);
        return id;
    }

    // Runs the whole scenario and returns the root's status as Alice sees it.
    debate::ClaimStatus rootStatusForAliceAfterConceding() {
        DebateHandler::AddDebate("Registry propagation", alice_, *debate_);
        const int debateId = 1;
        MoveUserHandler::EnterDebate(debateId, alice_, *debate_);
        const int root = 1;

        AddClaimHandler::OpenAddChildClaim(alice_, *debate_);
        AddClaimHandler::AddClaimUnderClaim({"the one reason"}, {"a reason"}, "EVIDENTIAL",
                                            alice_, *debate_);
        const int reason = debate_->findChildrenIds(root).back();

        MoveUserHandler::EnterDebate(debateId, bob_, *debate_);
        MoveUserHandler::GoToClaim(reason, bob_, *debate_);
        ChallengeHandler::StartChallengeClaim(bob_, *debate_);
        ChallengeHandler::AddClaimToBeChallenged(reason, bob_, *debate_);
        ChallengeHandler::OpenAddChallenge(bob_, *debate_);
        ChallengeHandler::SubmitChallengeClaim({"that reason is wrong"}, {"challenge"},
                                               "EVIDENTIAL", bob_, *debate_);

        int challengeLink = -1;
        for (const auto& row : debate_->getLinksForDebate(debateId)) {
            if (std::get<5>(row) == static_cast<int>(debate::LinkType::CHALLENGE) &&
                std::get<2>(row) == reason) {
                challengeLink = std::get<0>(row);
            }
        }
        EXPECT_GT(challengeLink, 0);
        ChallengeHandler::ConcedeChallenge(alice_, challengeLink, *debate_);

        // The conceded reason is FALSE for Alice under any registry -- that is
        // the concession itself, not propagation.
        const auto& reasonStatuses = debate_->getClaimById(reason).user_statuses();
        EXPECT_EQ(reasonStatuses.at("alice"), debate::ClaimStatus::FALSE_CLAIM);

        const auto& rootStatuses = debate_->getClaimById(root).user_statuses();
        auto it = rootStatuses.find("alice");
        return it == rootStatuses.end() ? debate::ClaimStatus::UNDETERMINED : it->second;
    }

    std::string dbPath_;
    Database* db_ = nullptr;
    DatabaseWrapper* wrapper_ = nullptr;
    DebateWrapper* debate_ = nullptr;
    int alice_ = 0;
    int bob_ = 0;
};

TEST_F(PropagationObeysRegistryTest, UnderCollapseAConcededReasonSinksTheRoot) {
    use(shippedRegistry());
    EXPECT_EQ(rootStatusForAliceAfterConceding(), debate::ClaimStatus::FALSE_CLAIM);
}

TEST_F(PropagationObeysRegistryTest, UnderUnsupportAConcededReasonDoesNot) {
    write(R"(
bases { name: "EVIDENTIAL" propagation: UNSUPPORT is_default: true }
)");
    // Not asserting UNDETERMINED: the live engine starts from the stored
    // status and only overrides when a rule fires (see StatusEngine.h on why
    // exact parity is impossible), so the root keeps whatever it had. What the
    // registry guarantees is that the concession no longer refutes it.
    EXPECT_NE(rootStatusForAliceAfterConceding(), debate::ClaimStatus::FALSE_CLAIM);
}

} // namespace
