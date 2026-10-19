#include <gtest/gtest.h>

#include "../../backend/src/utils/TutorialLibrary.h"
#include "debate_event.pb.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <set>
#include <string>

namespace {

// Writes scratch tutorial files and points the library at them, so these tests
// never depend on -- or disturb -- the scripts that ship in backend/tutorials.
class TutorialLibraryTest : public ::testing::Test {
protected:
    void SetUp() override {
        dir_ = std::filesystem::temp_directory_path() /
               ("tutorial_test_" + std::to_string(
                   reinterpret_cast<uintptr_t>(this)));
        std::filesystem::remove_all(dir_);
        std::filesystem::create_directories(dir_);
#ifdef _WIN32
        _putenv_s("DEBATE_TUTORIALS_DIR", dir_.string().c_str());
#else
        setenv("DEBATE_TUTORIALS_DIR", dir_.string().c_str(), 1);
#endif
    }

    void TearDown() override {
#ifdef _WIN32
        _putenv_s("DEBATE_TUTORIALS_DIR", "");
        _putenv_s("DEBATE_TUTORIAL", "");
#else
        unsetenv("DEBATE_TUTORIALS_DIR");
        unsetenv("DEBATE_TUTORIAL");
#endif
        std::filesystem::remove_all(dir_);
    }

    void write(const std::string& name, const std::string& contents) {
        std::ofstream out(dir_ / (name + ".pbtxt"));
        out << contents;
    }

    std::filesystem::path dir_;
};

const char* kMinimalScript = R"(
name: "demo"
title: "A demo"
steps {
  actions { username: "YOU" event_type: "CREATE_DEBATE" debate_topic: "T" }
  title: "First"
  body: "Something happened."
}
)";

TEST_F(TutorialLibraryTest, LoadsAValidScript) {
    write("demo", kMinimalScript);
    bool ok = false;
    std::string error;
    const tutorial::Tutorial loaded = tutorials::Load("demo", ok, error);
    ASSERT_TRUE(ok) << error;
    EXPECT_EQ(loaded.title(), "A demo");
    ASSERT_EQ(loaded.steps_size(), 1);
    EXPECT_EQ(loaded.steps(0).title(), "First");
    ASSERT_EQ(loaded.steps(0).actions_size(), 1);
    EXPECT_EQ(loaded.steps(0).actions(0).username(), "YOU");
}

// The name reaches Load from a client message and decides which file is
// opened, so a name that could climb out of the directory has to be refused
// outright rather than cleaned up into one that still opens something.
TEST_F(TutorialLibraryTest, RefusesNamesThatEscapeTheDirectory) {
    for (const std::string& evil : {"../secret", "..\\secret", "sub/deep", "a/../../b"}) {
        bool ok = true;
        std::string error;
        tutorials::Load(evil, ok, error);
        EXPECT_FALSE(ok) << "accepted " << evil;
        EXPECT_NE(error.find("not allowed"), std::string::npos) << evil;
    }
}

TEST_F(TutorialLibraryTest, ReportsAMissingScript) {
    bool ok = true;
    std::string error;
    tutorials::Load("nope", ok, error);
    EXPECT_FALSE(ok);
    EXPECT_NE(error.find("Could not open"), std::string::npos);
}

TEST_F(TutorialLibraryTest, ReportsAScriptThatDoesNotParse) {
    write("broken", "this is not protobuf text format {{{");
    bool ok = true;
    std::string error;
    tutorials::Load("broken", ok, error);
    EXPECT_FALSE(ok);
    EXPECT_NE(error.find("not valid protobuf"), std::string::npos);
}

// A file that parses but says nothing would start a walkthrough with no steps
// in it, which shows as a guide box that can never advance.
TEST_F(TutorialLibraryTest, ReportsAScriptWithNoSteps) {
    write("empty", "name: \"empty\"\ntitle: \"Empty\"\n");
    bool ok = true;
    std::string error;
    tutorials::Load("empty", ok, error);
    EXPECT_FALSE(ok);
    EXPECT_NE(error.find("no steps"), std::string::npos);
}

TEST_F(TutorialLibraryTest, FillsTheNameFromTheFilenameWhenTheScriptOmitsIt) {
    write("unnamed", "title: \"T\"\nsteps { title: \"S\" }\n");
    bool ok = false;
    std::string error;
    const tutorial::Tutorial loaded = tutorials::Load("unnamed", ok, error);
    ASSERT_TRUE(ok) << error;
    EXPECT_EQ(loaded.name(), "unnamed");
}

TEST_F(TutorialLibraryTest, ListsEveryReadableScriptSorted) {
    write("bbb", kMinimalScript);
    write("aaa", kMinimalScript);
    const std::vector<tutorials::Listing> found = tutorials::List();
    ASSERT_EQ(found.size(), 2u);
    EXPECT_EQ(found[0].name, "aaa");
    EXPECT_EQ(found[1].name, "bbb");
}

// One unfinished file in the directory should cost you that one walkthrough,
// not every walkthrough -- List feeds the home page.
TEST_F(TutorialLibraryTest, SkipsUnparseableFilesInsteadOfFailingTheList) {
    write("good", kMinimalScript);
    write("bad", "{{{ not a tutorial");
    const std::vector<tutorials::Listing> found = tutorials::List();
    ASSERT_EQ(found.size(), 1u);
    EXPECT_EQ(found[0].name, "good");
}

TEST_F(TutorialLibraryTest, IgnoresFilesThatAreNotPbtxt) {
    write("real", kMinimalScript);
    std::ofstream(dir_ / "notes.txt") << "ignore me";
    const std::vector<tutorials::Listing> found = tutorials::List();
    ASSERT_EQ(found.size(), 1u);
    EXPECT_EQ(found[0].name, "real");
}

TEST_F(TutorialLibraryTest, DefaultIsTheFirstScriptByName) {
    write("zebra", kMinimalScript);
    write("alpha", kMinimalScript);
    EXPECT_EQ(tutorials::DefaultName(), "alpha");
}

TEST_F(TutorialLibraryTest, DefaultCanBeOverriddenByEnvironment) {
    write("alpha", kMinimalScript);
    write("zebra", kMinimalScript);
#ifdef _WIN32
    _putenv_s("DEBATE_TUTORIAL", "zebra");
#else
    setenv("DEBATE_TUTORIAL", "zebra", 1);
#endif
    EXPECT_EQ(tutorials::DefaultName(), "zebra");
}

TEST_F(TutorialLibraryTest, AnEmptyDirectoryYieldsNoDefaultRatherThanCrashing) {
    EXPECT_TRUE(tutorials::List().empty());
    EXPECT_TRUE(tutorials::DefaultName().empty());
}

// ---------------------------------------------------------------------------
// The scripts that actually ship.
//
// These are hand-edited data files with no compiler behind them, so a typo in
// one is invisible until somebody clicks the button and the walkthrough stops
// on a step it cannot replay. That is the worst place to find out. Everything
// checkable without a database is checked here instead.
// ---------------------------------------------------------------------------
class ShippedTutorialsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Located relative to this source file so the test does not depend on
        // where the runner is invoked from.
        const std::filesystem::path here(__FILE__);
        dir_ = here.parent_path().parent_path().parent_path() / "backend" / "tutorials";
#ifdef _WIN32
        _putenv_s("DEBATE_TUTORIALS_DIR", dir_.string().c_str());
#else
        setenv("DEBATE_TUTORIALS_DIR", dir_.string().c_str(), 1);
#endif
    }
    void TearDown() override {
#ifdef _WIN32
        _putenv_s("DEBATE_TUTORIALS_DIR", "");
#else
        unsetenv("DEBATE_TUTORIALS_DIR");
#endif
    }
    std::filesystem::path dir_;
};

TEST_F(ShippedTutorialsTest, AtLeastOneShipsAndAllOfThemParse) {
    ASSERT_TRUE(std::filesystem::is_directory(dir_)) << dir_.string();
    const std::vector<tutorials::Listing> found = tutorials::List();
    ASSERT_FALSE(found.empty()) << "no tutorials in " << dir_.string();

    // List() silently skips files it cannot parse, so a script that broke
    // would show up as an absence rather than a failure. Count the files too.
    int pbtxtFiles = 0;
    for (const auto& entry : std::filesystem::directory_iterator(dir_)) {
        if (entry.is_regular_file() && entry.path().extension() == ".pbtxt") {
            pbtxtFiles++;
        }
    }
    EXPECT_EQ(static_cast<int>(found.size()), pbtxtFiles)
        << "a .pbtxt in backend/tutorials did not parse";
}

TEST_F(ShippedTutorialsTest, EveryStepIsReplayableAndExplained) {
    for (const tutorials::Listing& listing : tutorials::List()) {
        bool ok = false;
        std::string error;
        const tutorial::Tutorial script = tutorials::Load(listing.name, ok, error);
        ASSERT_TRUE(ok) << listing.name << ": " << error;
        EXPECT_FALSE(script.title().empty()) << listing.name << " has no title";

        // Claims are numbered from 1 in creation order. Tracking that count
        // here catches a claim_id pointing at a claim the script has not
        // created yet -- which at runtime silently navigates to whatever claim
        // holds that id, or to nothing.
        int claimsCreated = 0;
        bool sawViewer = false;

        for (int s = 0; s < script.steps_size(); ++s) {
            const tutorial::TutorialStep& step = script.steps(s);
            const std::string where = listing.name + " step " + std::to_string(s + 1);

            EXPECT_FALSE(step.title().empty()) << where << " has no title";
            EXPECT_GT(step.body_size(), 0) << where << " explains nothing";

            for (const debate_test::TestAction& action : step.actions()) {
                EXPECT_FALSE(action.username().empty()) << where << " has an action with no user";
                if (action.username() == "YOU") {
                    sawViewer = true;
                }

                // CREATE_USER is a runner-level operation, not a DebateEvent.
                if (action.event_type() != "CREATE_USER") {
                    debate_event::EventType parsed;
                    EXPECT_TRUE(debate_event::EventType_Parse(action.event_type(), &parsed))
                        << where << ": unknown event_type '" << action.event_type() << "'";
                }

                if (action.claim_id() > 0) {
                    EXPECT_LE(action.claim_id(), claimsCreated)
                        << where << " refers to claim " << action.claim_id()
                        << " but only " << claimsCreated << " exist by then";
                }

                if (action.event_type() == "CREATE_DEBATE") {
                    claimsCreated += 1;  // the root claim
                } else if (action.event_type() == "ADD_CHILD_CLAIM" ||
                           action.event_type() == "SUBMIT_CHALLENGE_CLAIM") {
                    claimsCreated += 1 + action.extra_claim_texts_size();
                }
            }
        }

        // A walkthrough where the reader never acts is one they watch from
        // outside: none of the statuses on screen would be theirs.
        EXPECT_TRUE(sawViewer) << listing.name << " never has the reader do anything";
    }
}

} // namespace
