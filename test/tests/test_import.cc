// test_import.cc -- an admin's "Import debate".
//
//   AdminsTest          who DEBATE_ADMINS names
//   DebateImportTest    reading the pasted text: the outline (nesting,
//                       defaults, errors with line numbers, limits) and the
//                       scenario file (what it may not do)
//   ImportFlowTest      through the moderator: only admins, Preview writes
//                       nothing, Publish builds the debate and opens it

#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "debate.pb.h"
#include "debate_event.pb.h"
#include "user.pb.h"
#include "database/sqlite/Database.h"
#include "debateModerator/DebateModerator.h"
#include "utils/Admins.h"
#include "utils/AiAvailability.h"
#include "utils/DebateImport.h"
#include "utils/DebateWrapper.h"

#ifdef _WIN32
#include <process.h>
#define GETPID _getpid
#else
#include <unistd.h>
#define GETPID getpid
#endif

namespace {

void setEnv(const char* name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    if (value.empty()) unsetenv(name); else setenv(name, value.c_str(), 1);
#endif
}

void removeDb(const std::string& path) {
    std::remove(path.c_str());
    std::remove((path + "-wal").c_str());
    std::remove((path + "-shm").c_str());
}

std::string readRepoFile(const std::string& relative) {
    const std::filesystem::path root =
        std::filesystem::path(__FILE__).parent_path().parent_path().parent_path();
    std::ifstream in(root / relative, std::ios::binary);
    std::stringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

// The actions of a plan, as "EVENT[:claim_id]" for the navigation steps.
std::vector<const debate_test::TestAction*> actionsOf(const debate_import::Plan& plan, const std::string& type) {
    std::vector<const debate_test::TestAction*> out;
    for (const auto& step : plan.scenario.steps()) {
        if (step.action().event_type() == type) out.push_back(&step.action());
    }
    return out;
}

bool anyContains(const std::vector<std::string>& lines, const std::string& needle) {
    for (const std::string& l : lines) {
        if (l.find(needle) != std::string::npos) return true;
    }
    return false;
}

const char* kOutline = R"(# A small test debate
Cats are better pets than dogs | Alice | https://example.com/cats | "Cats win." | | a note on the root
  + Cats need less daily care | Bob | https://example.com/care | "No walks needed." | EVIDENTIAL | care note
    - Cats still need a litter box cleaned daily | Carol | https://example.com/litter | "Daily." | evidential
  - Dogs are more loyal | Dan
	+ A tab-indented support of the dog claim
)";

}  // namespace

// ===========================================================================
// Admins
// ===========================================================================

TEST(AdminsTest, ListIsCommaSeparatedTrimmedAndCaseInsensitive) {
    EXPECT_TRUE(admins::IsAdminIn("roger, Phase2-Tester ,", "phase2-tester"));
    EXPECT_TRUE(admins::IsAdminIn("roger,phase2-tester", "ROGER"));
    EXPECT_FALSE(admins::IsAdminIn("roger,phase2-tester", "rog"));
    EXPECT_FALSE(admins::IsAdminIn("", "roger"));
    EXPECT_FALSE(admins::IsAdminIn(" , ", ""));
}

TEST(AdminsTest, AVisitorIsNeverAnAdmin) {
    EXPECT_FALSE(admins::IsAdminIn("visitor-0000000a", "visitor-0000000a"));
}

TEST(AdminsTest, ReadsDebateAdmins) {
    setEnv("DEBATE_ADMINS", "Alice");
    EXPECT_TRUE(admins::IsAdmin("alice"));
    EXPECT_FALSE(admins::IsAdmin("bob"));
    setEnv("DEBATE_ADMINS", "");
    EXPECT_FALSE(admins::IsAdmin("alice"));
}

// DEBATE_AI=admins: the AI only for the names DEBATE_ADMINS lists.
TEST(AdminsTest, AiForAdminsOnly) {
    setEnv("DEBATE_ADMINS", "Alice");
    setEnv("DEBATE_AI", "admins");
    EXPECT_TRUE(ai_availability::EnabledFor(std::string("alice")));
    EXPECT_FALSE(ai_availability::EnabledFor(std::string("bob")));
    EXPECT_FALSE(ai_availability::EnabledFor(std::string("visitor-0000000a")));
    EXPECT_TRUE(ai_availability::EnabledFor(true));
    EXPECT_FALSE(ai_availability::EnabledFor(false));
    setEnv("DEBATE_AI", "off");
    EXPECT_FALSE(ai_availability::EnabledFor(std::string("alice")));
    setEnv("DEBATE_AI", "");
    EXPECT_TRUE(ai_availability::EnabledFor(std::string("bob")));
    setEnv("DEBATE_ADMINS", "");
}

// ===========================================================================
// Reading the text
// ===========================================================================

TEST(DebateImportTest, OutlineNestsByIndentation) {
    const debate_import::Plan plan = debate_import::Read(kOutline);
    ASSERT_TRUE(plan.ok()) << (plan.errors.empty() ? "" : plan.errors[0]);
    EXPECT_EQ(plan.format, "outline");
    EXPECT_EQ(plan.root, "Cats are better pets than dogs");
    EXPECT_EQ(plan.claims, 5);
    EXPECT_EQ(plan.supports, 2);
    EXPECT_EQ(plan.challenges, 2);

    // Each claim goes to its parent first: claims are numbered 1.. in order.
    const auto gotos = actionsOf(plan, "GO_TO_CLAIM");
    ASSERT_EQ(gotos.size(), 4u);
    EXPECT_EQ(gotos[0]->claim_id(), 1);  // care -> root
    EXPECT_EQ(gotos[1]->claim_id(), 2);  // litter -> care
    EXPECT_EQ(gotos[2]->claim_id(), 1);  // loyal -> root
    EXPECT_EQ(gotos[3]->claim_id(), 4);  // the tab line -> loyal (a tab is 4 spaces)

    const auto adds = actionsOf(plan, "ADD_CHILD_CLAIM");
    const auto challenges = actionsOf(plan, "SUBMIT_CHALLENGE_CLAIM");
    ASSERT_EQ(adds.size(), 2u);
    ASSERT_EQ(challenges.size(), 2u);
    EXPECT_EQ(challenges[0]->claim_text(), "Cats still need a litter box cleaned daily");
    EXPECT_EQ(challenges[0]->basis(), "EVIDENTIAL") << "basis is upper-cased";
    EXPECT_EQ(actionsOf(plan, "ADD_CLAIM_TO_BE_CHALLENGED")[0]->claim_id(), 2);
    EXPECT_EQ(plan.voices, (std::vector<std::string>{"Alice", "Bob", "Carol", "Dan"}));
    // The first step, then the debate is created, joined and entered.
    EXPECT_EQ(plan.scenario.steps(0).action().event_type(), "CREATE_USER");
    EXPECT_EQ(plan.scenario.steps(1).action().event_type(), "CREATE_DEBATE");
    EXPECT_EQ(plan.scenario.steps(2).action().debate_id(), 1);
    EXPECT_EQ(plan.stepLines.size(), static_cast<size_t>(plan.scenario.steps_size()));
    EXPECT_EQ(plan.stepLines[1], 2);
}

TEST(DebateImportTest, OutlineFieldsAndDefaults) {
    const debate_import::Plan plan = debate_import::Read(kOutline);
    const debate_test::TestAction& root = *actionsOf(plan, "CREATE_DEBATE")[0];
    EXPECT_EQ(root.voice(), "Alice");
    EXPECT_EQ(root.source_url(), "https://example.com/cats");
    EXPECT_EQ(root.source_excerpt(), "Cats win.") << "surrounding quotes stripped";

    const auto adds = actionsOf(plan, "ADD_CHILD_CLAIM");
    EXPECT_EQ(adds[0]->claim_description(), "care note") << "the note is the description";
    EXPECT_EQ(adds[0]->basis(), "EVIDENTIAL");
    // Only the text: the importer speaks, no source, no basis.
    EXPECT_EQ(adds[1]->claim_text(), "A tab-indented support of the dog claim");
    EXPECT_EQ(adds[1]->voice(), "");
    EXPECT_EQ(adds[1]->source_url(), "");
    EXPECT_EQ(adds[1]->basis(), "");
    // The root's note has nowhere to go: said, not an error.
    EXPECT_TRUE(anyContains(plan.warnings, "Line 2: the root's note"));
}

TEST(DebateImportTest, OutlineErrorsNameTheirLine) {
    const debate_import::Plan plan = debate_import::Read(
        "Root claim\n"
        "  + Fine support | V | https://x.example | \"q\" | VIBES\n"
        "  Missing marker\n"
        "  + Bad source | V | example.com\n"
        "+ Same indentation as the root\n"
        "  - | Voice only\n");
    EXPECT_FALSE(plan.ok());
    EXPECT_TRUE(anyContains(plan.errors, "Line 2: \"VIBES\" is not a basis"));
    EXPECT_TRUE(anyContains(plan.errors, "Line 3: start the line with +"));
    EXPECT_TRUE(anyContains(plan.errors, "Line 4: the source"));
    EXPECT_TRUE(anyContains(plan.errors, "Line 5: there is no claim above it"));
    EXPECT_TRUE(anyContains(plan.errors, "Line 6: the claim's text is empty"));
}

TEST(DebateImportTest, OutlineRootHasNoMarkerAndTextNeedsAClaim) {
    EXPECT_TRUE(anyContains(debate_import::Read("+ Root with a marker\n").errors, "Line 1: the first claim"));
    EXPECT_TRUE(anyContains(debate_import::Read("# only a comment\n\n").errors, "There is no claim"));
    EXPECT_TRUE(anyContains(debate_import::Read("  \n").errors, "Paste a scenario file or an outline"));
    EXPECT_TRUE(anyContains(debate_import::Read("Root\n  + a | b | c | d | e | f | g | h\n").errors,
                            "Line 2: has 8 fields"));
}

// The seventh field is a picture of the source, by web address.
TEST(DebateImportTest, OutlineImageIsAWebAddress) {
    const debate_import::Plan ok = debate_import::Read(
        "Root\n  + A claim | Voice | https://a.example/post | \"words\" | EVIDENTIAL | note | https://a.example/shot.png\n");
    ASSERT_TRUE(ok.ok());
    bool found = false;
    for (const auto& step : ok.scenario.steps()) {
        if (step.action().source_image_url() == "https://a.example/shot.png") found = true;
    }
    EXPECT_TRUE(found);
    EXPECT_TRUE(anyContains(debate_import::Read("Root\n  + A claim | | | | | | shot.png\n").errors,
                            "Line 2: the image \"shot.png\" is not a web address"));
}

TEST(DebateImportTest, Limits) {
    std::string big(debate_import::kMaxBytes + 1, 'x');
    const debate_import::Plan tooBig = debate_import::Read(big);
    EXPECT_FALSE(tooBig.ok());
    EXPECT_TRUE(anyContains(tooBig.errors, "at most 300 KB"));

    std::string many = "Root\n";
    for (int i = 0; i < debate_import::kMaxClaims; ++i) many += "  + c" + std::to_string(i) + "\n";
    const debate_import::Plan tooMany = debate_import::Read(many);
    EXPECT_EQ(tooMany.claims, debate_import::kMaxClaims + 1);
    EXPECT_TRUE(anyContains(tooMany.errors, "at most 500"));

    many = "Root\n";
    for (int i = 0; i < debate_import::kMaxClaims - 1; ++i) many += "  + c" + std::to_string(i) + "\n";
    EXPECT_TRUE(debate_import::Read(many).ok());
}

TEST(DebateImportTest, ScenarioFilesAreDetectedAndChecked) {
    const std::string fauci = readRepoFile("test/scenarios/FauciCaseSourced.pbtxt");
    ASSERT_FALSE(fauci.empty());
    const debate_import::Plan plan = debate_import::Read(fauci);
    EXPECT_EQ(plan.format, "scenario");
    EXPECT_TRUE(plan.ok()) << (plan.errors.empty() ? "" : plan.errors[0]);
    EXPECT_EQ(plan.actor, "guest");
    EXPECT_EQ(plan.claims, 1 + 44 + 23);
    EXPECT_EQ(plan.supports, 44);
    EXPECT_EQ(plan.challenges, 23);
    EXPECT_FALSE(plan.voices.empty());

    // A broken file reports the parser's words, not outline errors.
    const debate_import::Plan broken = debate_import::Read("name: \"x\"\nsteps { action { username: \"guest\" \n");
    EXPECT_EQ(broken.format, "scenario");
    EXPECT_FALSE(broken.ok());
    EXPECT_TRUE(anyContains(broken.errors, "Line ")) << broken.errors[0];
}

TEST(DebateImportTest, ScenarioMayOnlyBuildItsOwnDebate) {
    const std::string head =
        "steps { action { username: \"guest\" event_type: \"CREATE_DEBATE\" debate_topic: \"Root\" } }\n";
    auto errorsOf = [&](const std::string& rest) { return debate_import::Read(head + rest).errors; };

    EXPECT_TRUE(anyContains(errorsOf("steps { action { username: \"guest\" event_type: \"DELETE_DEBATE\" debate_id: 3 } }"),
                            "cannot replay"));
    EXPECT_TRUE(anyContains(errorsOf("steps { action { username: \"guest\" event_type: \"ENTER_DEBATE\" debate_id: 7 } }"),
                            "debate_id must be 1"));
    EXPECT_TRUE(anyContains(errorsOf("steps { action { username: \"guest\" event_type: \"GO_TO_CLAIM\" claim_id: 2 } }"),
                            "claim_id 2 is not one of the 1"));
    EXPECT_TRUE(anyContains(errorsOf("steps { action { username: \"other\" event_type: \"GO_TO_CLAIM\" claim_id: 1 } }"),
                            "acts as 2 users"));
    EXPECT_TRUE(anyContains(errorsOf("steps { action { username: \"guest\" event_type: \"ADD_CHILD_CLAIM\" claim_text: \"x\" basis: \"NOPE\" } }"),
                            "is not a basis"));
    EXPECT_TRUE(anyContains(errorsOf("steps { action { username: \"Mallory\" event_type: \"CREATE_USER\" } }"),
                            "creates the account"));
    EXPECT_TRUE(debate_import::Read(head + "steps { action { username: \"guest\" event_type: \"GO_TO_CLAIM\" claim_id: 1 } }").ok());
}

// ===========================================================================
// Through the moderator
// ===========================================================================

class ImportFlowTest : public ::testing::Test {
protected:
    void SetUp() override {
        path_ = "test_import_" + std::to_string(GETPID()) + ".sqlite3";
        removeDb(path_);
        setEnv("DB_PATH", path_);
        setEnv("DEBATE_ADMINS", "Admin");
        db_ = new Database(path_);
        moderator_ = new DebateModerator(*db_);
        admin_ = moderator_->createUserIfNotExist("admin");
        bob_ = moderator_->createUserIfNotExist("bob");
        visitor_ = moderator_->createUserIfNotExist("visitor-0000abcd");
    }
    void TearDown() override {
        delete moderator_;
        delete db_;
        setEnv("DB_PATH", "");
        setEnv("DEBATE_ADMINS", "");
        removeDb(path_);
    }

    debate_event::DebateEvent as(int userId, debate_event::EventType type, const std::string& text = "") {
        debate_event::DebateEvent e;
        e.mutable_user()->set_user_id(userId);
        e.mutable_user()->set_username(moderator_->getDebateWrapper().getUserProtobuf(userId).username());
        e.mutable_user()->set_is_logged_in(true);
        e.set_type(type);
        if (type == debate_event::IMPORT_PREVIEW || type == debate_event::IMPORT_PUBLISH) {
            e.mutable_import_debate()->set_text(text);
        }
        return e;
    }
    void send(debate_event::DebateEvent e) { moderator_->handleRequest(e); }
    user::User userOf(int id) { return moderator_->getDebateWrapper().getUserProtobuf(id); }
    size_t debateCount() {
        return moderator_->getDebateWrapper().getUserDebateIds(admin_).size();
    }

    std::string path_;
    Database* db_ = nullptr;
    DebateModerator* moderator_ = nullptr;
    int admin_ = 0, bob_ = 0, visitor_ = 0;
};

TEST_F(ImportFlowTest, OnlyAdminsReachTheImportView) {
    send(as(bob_, debate_event::IMPORT_OPEN));
    EXPECT_FALSE(userOf(bob_).engagement().home_view().import_open());
    send(as(bob_, debate_event::IMPORT_PREVIEW, kOutline));
    EXPECT_FALSE(userOf(bob_).engagement().home_view().import_preview().done());
    send(as(visitor_, debate_event::IMPORT_OPEN));
    EXPECT_FALSE(userOf(visitor_).engagement().home_view().import_open());

    // Even a list naming the visitor does not make them one.
    setEnv("DEBATE_ADMINS", "admin,visitor-0000abcd");
    send(as(visitor_, debate_event::IMPORT_OPEN));
    EXPECT_FALSE(userOf(visitor_).engagement().home_view().import_open());

    send(as(admin_, debate_event::IMPORT_OPEN));
    const auto view = userOf(admin_).engagement().home_view();
    EXPECT_TRUE(view.import_open());
    EXPECT_EQ(view.profile_user_id(), admin_) << "on their own profile";
}

TEST_F(ImportFlowTest, NonAdminPublishWritesNothing) {
    const int users = moderator_->getUserId("Carol");
    send(as(bob_, debate_event::IMPORT_PUBLISH, kOutline));
    EXPECT_TRUE(moderator_->getDebateWrapper().getUserDebateIds(bob_).empty());
    EXPECT_EQ(moderator_->getUserId("Carol"), users);
}

TEST_F(ImportFlowTest, PreviewWritesNothingAndListsNewVoices) {
    moderator_->createUserIfNotExist("Alice");
    send(as(admin_, debate_event::IMPORT_OPEN));
    send(as(admin_, debate_event::IMPORT_PREVIEW, kOutline));
    EXPECT_EQ(debateCount(), 0u);
    EXPECT_EQ(moderator_->getUserId("Carol"), -1) << "a voice is created by Publish, not Preview";

    const auto view = userOf(admin_).engagement().home_view();
    EXPECT_EQ(view.import_text(), kOutline);
    const auto& p = view.import_preview();
    EXPECT_TRUE(p.done());
    EXPECT_EQ(p.format(), "outline");
    EXPECT_EQ(p.claims(), 5);
    EXPECT_EQ(p.errors_size(), 0);
    EXPECT_EQ(p.voices_size(), 4);
    ASSERT_EQ(p.new_voices_size(), 3) << "Alice already exists";
    EXPECT_EQ(p.new_voices(0), "Bob");
}

TEST_F(ImportFlowTest, PublishBuildsTheDebateAndOpensIt) {
    send(as(admin_, debate_event::IMPORT_OPEN));
    send(as(admin_, debate_event::IMPORT_PREVIEW, kOutline));
    send(as(admin_, debate_event::IMPORT_PUBLISH, kOutline));

    const std::vector<int> ids = moderator_->getDebateWrapper().getUserDebateIds(admin_);
    ASSERT_EQ(ids.size(), 1u);
    const user::User admin = userOf(admin_);
    EXPECT_EQ(admin.engagement().current_action(), user_engagement::ACTION_DEBATING);
    EXPECT_EQ(admin.engagement().debating_info().debate_id(), ids[0]);
    EXPECT_FALSE(admin.engagement().home_view().import_open());
    EXPECT_TRUE(admin.engagement().home_view().import_text().empty()) << "cleared after publishing";

    DebateWrapper& dw = moderator_->getDebateWrapper();
    const auto blobs = dw.getStatementsForDebate(ids[0]);
    ASSERT_EQ(blobs.size(), 5u);
    bool sawLitter = false;
    for (const auto& blob : blobs) {
        debate::Claim c;
        ASSERT_TRUE(c.ParseFromArray(blob.data(), static_cast<int>(blob.size())));
        EXPECT_EQ(c.creator_id(), admin_) << "every step entered as the admin";
        if (c.sentence() == "Cats still need a litter box cleaned daily") {
            sawLitter = true;
            EXPECT_EQ(c.voice_id(), moderator_->getUserId("Carol"));
            ASSERT_EQ(c.sources_size(), 1);
            EXPECT_EQ(c.sources(0).url(), "https://example.com/litter");
            EXPECT_EQ(c.sources(0).excerpt(), "Daily.");
        }
    }
    EXPECT_TRUE(sawLitter);
    EXPECT_EQ(moderator_->getUserId("guest"), -1) << "the outline's actor is the admin, not a guest account";
}

TEST_F(ImportFlowTest, PublishOnlyWhatWasPreviewed) {
    send(as(admin_, debate_event::IMPORT_OPEN));
    // No preview yet: Publish previews instead.
    send(as(admin_, debate_event::IMPORT_PUBLISH, kOutline));
    EXPECT_EQ(debateCount(), 0u);
    EXPECT_TRUE(userOf(admin_).engagement().home_view().import_preview().done());

    // Edited after the preview: checked again, not published.
    const std::string edited = std::string(kOutline) + "  + One more\n";
    send(as(admin_, debate_event::IMPORT_PUBLISH, edited));
    EXPECT_EQ(debateCount(), 0u);
    const auto view = userOf(admin_).engagement().home_view();
    EXPECT_EQ(view.import_text(), edited);
    EXPECT_NE(view.import_preview().publish_error().find("changed after the preview"), std::string::npos);

    // With errors, never.
    send(as(admin_, debate_event::IMPORT_PREVIEW, "Root\n+ no parent\n"));
    send(as(admin_, debate_event::IMPORT_PUBLISH, "Root\n+ no parent\n"));
    EXPECT_EQ(debateCount(), 0u);
}

TEST_F(ImportFlowTest, CloseForgetsTheText) {
    send(as(admin_, debate_event::IMPORT_OPEN));
    send(as(admin_, debate_event::IMPORT_PREVIEW, kOutline));
    send(as(admin_, debate_event::IMPORT_CLOSE));
    const auto view = userOf(admin_).engagement().home_view();
    EXPECT_FALSE(view.import_open());
    EXPECT_TRUE(view.import_text().empty());
    EXPECT_FALSE(view.import_preview().done());
    EXPECT_EQ(view.profile_user_id(), admin_) << "back on the profile";
}

TEST_F(ImportFlowTest, ScenarioFileImportsAsTheAdmin) {
    const std::string dream = readRepoFile("test/scenarios/DreamCaseSourced.pbtxt");
    send(as(admin_, debate_event::IMPORT_OPEN));
    send(as(admin_, debate_event::IMPORT_PREVIEW, dream));
    ASSERT_EQ(userOf(admin_).engagement().home_view().import_preview().errors_size(), 0);
    send(as(admin_, debate_event::IMPORT_PUBLISH, dream));
    const std::vector<int> ids = moderator_->getDebateWrapper().getUserDebateIds(admin_);
    ASSERT_EQ(ids.size(), 1u);
    // One claim per root, support and challenge step in the file, counted
    // rather than hard-coded so editing the case doesn't break the test.
    size_t expected = 0;
    for (const char* step : {"CREATE_DEBATE\"", "ADD_CHILD_CLAIM\"", "SUBMIT_CHALLENGE_CLAIM\""}) {
        for (size_t at = dream.find(step); at != std::string::npos; at = dream.find(step, at + 1)) ++expected;
    }
    ASSERT_GT(expected, 0u);
    EXPECT_EQ(moderator_->getDebateWrapper().getStatementsForDebate(ids[0]).size(), expected);
    EXPECT_EQ(moderator_->getUserId("guest"), -1) << "guest was aliased to the admin";
}

TEST(DebateImportTest, TheDocumentedExampleReads) {
    const std::string doc = readRepoFile("docs/IMPORT_FORMAT.md");
    const std::string marker = "### A complete example";
    const size_t at = doc.find(marker);
    ASSERT_NE(at, std::string::npos);
    const size_t open = doc.find("```\n", at) + 4;
    const size_t close = doc.find("```", open);
    const debate_import::Plan plan = debate_import::Read(doc.substr(open, close - open));
    EXPECT_TRUE(plan.ok()) << (plan.errors.empty() ? "" : plan.errors[0]);
    EXPECT_EQ(plan.format, "outline");
    EXPECT_EQ(plan.claims, 5);
    EXPECT_EQ(plan.supports, 2);
    EXPECT_EQ(plan.challenges, 2);
    EXPECT_EQ(plan.voices.size(), 5u);
}

TEST_F(ImportFlowTest, AFailurePartWayIsReportedWithThePartialDebate) {
    // A support added without first entering the debate has no claim to
    // hang from, and the server drops it: every step "runs", but the debate
    // ends up short a claim.
    const std::string text =
        "steps { action { username: \"guest\" event_type: \"CREATE_DEBATE\" debate_topic: \"Root\" } }\n"
        "steps { action { username: \"guest\" event_type: \"ADD_CHILD_CLAIM\" claim_text: \"Yes\" } }\n";
    send(as(admin_, debate_event::IMPORT_OPEN));
    send(as(admin_, debate_event::IMPORT_PREVIEW, text));
    ASSERT_EQ(userOf(admin_).engagement().home_view().import_preview().errors_size(), 0);
    send(as(admin_, debate_event::IMPORT_PUBLISH, text));

    const std::vector<int> ids = moderator_->getDebateWrapper().getUserDebateIds(admin_);
    ASSERT_EQ(ids.size(), 1u);
    const user::User admin = userOf(admin_);
    EXPECT_EQ(admin.engagement().current_action(), user_engagement::ACTION_HOME) << "back on the import view";
    const auto& view = admin.engagement().home_view();
    EXPECT_TRUE(view.import_open());
    EXPECT_EQ(view.import_text(), text) << "kept, to fix and try again";
    const std::string error = view.import_preview().publish_error();
    EXPECT_NE(error.find("1 claims where 2 were expected"), std::string::npos) << error;
    EXPECT_NE(error.find("partial debate may exist: debate " + std::to_string(ids[0])), std::string::npos) << error;
}
