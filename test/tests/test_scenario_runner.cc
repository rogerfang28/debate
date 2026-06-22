// test_scenario_runner.cc — Data-driven test runner for TestScenario protobufs
//
// ARCHITECTURE OVERVIEW
// =====================
// This file implements a generic test runner that executes debate scenarios
// defined entirely in TestScenario protobuf messages. There is ZERO hardcoded
// state — no hardcoded users, no hardcoded debate IDs, no hardcoded topics.
// Everything flows through TestScenario.actions.
//
// HOW IT WORKS
// ============
// 1. Each TEST_F builds a TestScenario protobuf in memory
// 2. TestScenario.actions define what happens (user creation, debate events)
// 3. TestScenario.expectations define what should be true afterward
// 4. executeScenario() runs the actions, builds collection, checks expectations
//
// ACTION TYPES
// ============
// "CREATE_USER" — Special runner-level action. Calls createUserIfNotExist().
//                 Not a DebateEvent — handled before event forging.
// Any other string — Parsed as debate_event::EventType enum name.
//                   Forged into a DebateEvent and sent through handleRequest().
//
// EXPECTATION CHECK_TYPES
// =======================
// CLAIM_EXISTS      — Claim ID is present in the shared collection
// CLAIM_SENTENCE    — Claim's text matches expected string
// USER_VIEW         — Per-user status on a claim (TRUE/FALSE/UNDETERMINED)
// LINK_COUNT        — Number of links of a given type in collection
// LINK_EXISTS       — Specific link from→to with given type exists
// ENGAGEMENT_STATE  — User's current action (ACTION_DEBATING / ACTION_HOME)
// COLLECTION_SIZE   — Total number of claims in the collection
//
// Usage: debate_tests --gtest_filter="ScenarioRunner.*"

#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <map>
#include <iostream>
#include <fstream>

#include "debate.pb.h"
#include "debate_event.pb.h"
#include "user.pb.h"
#include "user_engagement.pb.h"
#include "moderator_to_vr.pb.h"
#include "test_scenario.pb.h"
#include "database/sqlite/Database.h"
#include "database/debate/DatabaseWrapper.h"
#include "utils/DebateWrapper.h"
#include "debateModerator/DebateModerator.h"
#include "debateModerator/buildResponse/debatePageResponse/BuildCollection.h"
#include "google/protobuf/text_format.h"

// Forward declaration — defined in test_step_view_layout.cc
void downloadPbtxt(int debate_id, const std::string& viewer_username = "A",
                   const std::string& output_path = "");

using debate_test::TestScenario;
using debate_test::TestAction;
using debate_test::TestExpectation;


// ===========================================================================
// SECTION 1: TEST FIXTURE
// ===========================================================================
// ScenarioRunner is the gtest fixture. Each TEST_F gets a fresh instance
// with its own DebateModerator and clean database.

class ScenarioRunner : public ::testing::Test {
protected:

    // -----------------------------------------------------------------------
    // SECTION 1a: Lifecycle (SetUp / TearDown)
    // -----------------------------------------------------------------------
    // SetUp: Creates a fresh SQLite DB and DebateModerator for each test.
    // TearDown: Cleans up the moderator and unsets the DB env var.

    void SetUp() override {
        db_path_ = "test_scenario.sqlite3";
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
        _putenv_s("DB_PATH", db_path_.c_str());
        moderator_ = new DebateModerator();
    }

    void TearDown() override {
        delete moderator_;
        _putenv_s("DB_PATH", "");
    }


    // -----------------------------------------------------------------------
    // SECTION 1b: User Management
    // -----------------------------------------------------------------------
    // getUserId: Resolves a username to a user_id. Creates the user on first
    // encounter via createUserIfNotExist(). Subsequent calls return the cached ID.
    // Users are only created when a CREATE_USER action or the first event for
    // that username is encountered.

    int getUserId(const std::string& username) {
        auto it = user_ids_.find(username);
        if (it != user_ids_.end()) return it->second;
        int id = moderator_->createUserIfNotExist(username);
        user_ids_[username] = id;
        return id;
    }


    // -----------------------------------------------------------------------
    // SECTION 2: STRING → ENUM PARSERS
    // -----------------------------------------------------------------------
    // Convert human-readable strings from the proto into C++ enums.

    debate_event::EventType parseEventType(const std::string& name) {
        debate_event::EventType type;
        if (debate_event::EventType_Parse(name, &type)) return type;
        return debate_event::NONE;
    }

    debate::ClaimStatus parseClaimStatus(const std::string& s) {
        if (s == "TRUE_CLAIM") return debate::ClaimStatus::TRUE_CLAIM;
        if (s == "FALSE_CLAIM") return debate::ClaimStatus::FALSE_CLAIM;
        return debate::ClaimStatus::UNDETERMINED;
    }

    user_engagement::EngagementAction parseEngagementAction(const std::string& s) {
        if (s == "ACTION_DEBATING") return user_engagement::ACTION_DEBATING;
        return user_engagement::ACTION_HOME;
    }

    debate::LinkType parseLinkType(const std::string& s) {
        if (s == "CHALLENGE") return debate::LinkType::CHALLENGE;
        return debate::LinkType::PARENT_CHILD;
    }


    // -----------------------------------------------------------------------
    // SECTION 3: EVENT FORGING
    // -----------------------------------------------------------------------
    // forgeEvent: Converts a TestAction into a DebateEvent protobuf.
    // Does NOT handle CREATE_USER — that's handled separately in executeScenario().

    debate_event::DebateEvent forgeEvent(const TestAction& action) {
        int user_id = getUserId(action.username());
        debate_event::EventType type = parseEventType(action.event_type());

        debate_event::DebateEvent event;
        event.mutable_user()->set_user_id(user_id);
        event.mutable_user()->set_username(action.username());
        event.mutable_user()->set_is_logged_in(true);
        event.set_type(type);

        switch (type) {
            case debate_event::CREATE_DEBATE:
                event.mutable_create_debate()->set_debate_topic(
                    action.debate_topic().empty() ? "Test Debate" : action.debate_topic());
                break;
            case debate_event::ENTER_DEBATE:
                event.mutable_enter_debate()->set_debate_id(action.debate_id());
                break;
            case debate_event::JOIN_DEBATE:
                event.mutable_join_debate()->set_debate_id(action.debate_id());
                break;
            case debate_event::GO_TO_CLAIM:
                event.mutable_go_to_claim()->set_claim_id(action.claim_id());
                break;
            case debate_event::OPEN_ADD_CHILD_CLAIM:
                break;
            case debate_event::ADD_CHILD_CLAIM:
                event.mutable_add_child_claim()->set_claim(action.claim_text());
                event.mutable_add_child_claim()->set_description(
                    action.claim_description().empty() ? "test" : action.claim_description());
                break;
            case debate_event::SUBMIT_CHALLENGE_CLAIM:
                event.mutable_submit_challenge_claim()->set_challenge_sentence(action.claim_text());
                break;
            case debate_event::CONCEDE_CHALLENGE:
                break;
            default:
                break;
        }
        return event;
    }


    // -----------------------------------------------------------------------
    // SECTION 4: COLLECTION BUILDER
    // -----------------------------------------------------------------------
    // Instead of sending a NONE event to each user (wasteful, doesn't match
    // production), we call BuildCollection::BuildForDebateAndUsers() directly.
    // This is the same function the backend uses for the virtual renderer.
    // One call returns a single Collection with all claims, links, and
    // per-user statuses for all users.

    debate::Collection buildCollectionForAllUsers(int debate_id) {
        std::vector<int> user_ids;
        for (const auto& kv : user_ids_) {
            user_ids.push_back(kv.second);
        }
        return BuildCollection::BuildForDebateAndUsers(debate_id, user_ids, moderator_->getDebateWrapper());
    }


    // -----------------------------------------------------------------------
    // SECTION 5: COLLECTION QUERY HELPERS
    // -----------------------------------------------------------------------
    // These extract data from a single Collection protobuf (shared across all users).
    // Each claim has a user_statuses map: username → TRUE/FALSE/UNDETERMINED.

    debate::Claim getClaim(const debate::Collection& collection, int claim_id) {
        auto it = collection.claims_by_id().find(claim_id);
        if (it != collection.claims_by_id().end()) return it->second;
        return debate::Claim();
    }

    debate::ClaimStatus getUserView(const debate::Collection& collection,
                                     int claim_id, const std::string& username) {
        debate::Claim c = getClaim(collection, claim_id);
        if (c.id() == 0) return debate::ClaimStatus::UNDETERMINED;
        auto it = c.user_statuses().find(username);
        if (it != c.user_statuses().end()) return it->second;
        return debate::ClaimStatus::UNDETERMINED;
    }

    int countLinks(const debate::Collection& collection, debate::LinkType type) {
        int count = 0;
        for (const auto& entry : collection.links_by_id()) {
            if (entry.second.link_type() == type) count++;
        }
        return count;
    }

    bool linkExists(const debate::Collection& collection, debate::LinkType type,
                    int from_claim_id, int to_claim_id) {
        for (const auto& entry : collection.links_by_id()) {
            const debate::Link& link = entry.second;
            if (link.link_type() == type &&
                link.connect_from() == from_claim_id &&
                link.connect_to() == to_claim_id) {
                return true;
            }
        }
        return false;
    }

    // Find the Nth claim ID that is the "from" end of a link with the given type
    // targeting a specific claim. Used to find challenge claims by excluding
    // already-known ones. skip_ids are IDs to skip (e.g. already-found challenges).
    int findNthChallengeClaimId(const debate::Collection& collection, int target_claim_id,
                                 debate::LinkType link_type, int n, const std::vector<int>& skip_ids) {
        int found = 0;
        for (const auto& entry : collection.links_by_id()) {
            const debate::Link& link = entry.second;
            if (link.link_type() != link_type || link.connect_to() != target_claim_id) continue;
            int from_id = link.connect_from();
            bool skip = false;
            for (int sid : skip_ids) { if (from_id == sid) { skip = true; break; } }
            if (skip) continue;
            if (++found == n) return from_id;
        }
        return -1;
    }


    // -----------------------------------------------------------------------
    // SECTION 6: SCENARIO EXECUTOR
    // -----------------------------------------------------------------------
    // executeScenario: Runs a TestScenario end-to-end.
    //
    // Phase 1 — Execute actions:
    //   CREATE_USER → calls getUserId()
    //   All others → forgeEvent() → handleRequest()
    //   Stores each user's last response (needed for ENGAGEMENT_STATE).
    //
    // Phase 2 — Build collection:
    //   Single call to BuildCollection::BuildForDebateAndUsers().
    //
    // Phase 3 — Check expectations:
    //   ENGAGEMENT_STATE → checked against stored per-user response
    //   All others → checked against the shared collection

    void executeScenario(const TestScenario& scenario) {
        // Phase 1: Execute all actions
        std::map<std::string, moderator_to_vr::ModeratorToVRMessage> last_responses;
        for (const auto& action : scenario.actions()) {
            if (action.event_type() == "CREATE_USER") {
                getUserId(action.username());
                continue;
            }
            int user_id = getUserId(action.username());
            debate_event::DebateEvent event = forgeEvent(action);
            auto resp = moderator_->handleRequest(event);
            last_responses[action.username()] = resp;
        }

        // Phase 2: Build collection (find debate_id from actions)
        int debate_id = 0;
        for (const auto& action : scenario.actions()) {
            if ((action.event_type() == "ENTER_DEBATE" || action.event_type() == "JOIN_DEBATE")
                && action.debate_id() > 0) {
                debate_id = action.debate_id();
                break;
            }
        }
        debate::Collection collection;
        if (debate_id > 0) {
            collection = buildCollectionForAllUsers(debate_id);
        }

        // Phase 3: Verify expectations
        for (const auto& exp : scenario.expectations()) {
            const std::string& username = exp.username();

            if (exp.check_type() == "ENGAGEMENT_STATE") {
                auto resp_it = last_responses.find(username);
                ASSERT_TRUE(resp_it != last_responses.end())
                    << "[" << scenario.name() << "] ENGAGEMENT_STATE unknown user: " << username;
                auto actual = resp_it->second.user().engagement().current_action();
                auto expected = parseEngagementAction(exp.expected_action());
                EXPECT_EQ(actual, expected)
                    << "[" << scenario.name() << "] ENGAGEMENT_STATE " << username;
                continue;
            }

            if (exp.check_type() == "CLAIM_EXISTS") {
                EXPECT_GT(getClaim(collection, exp.claim_id()).id(), 0)
                    << "[" << scenario.name() << "] CLAIM_EXISTS " << exp.claim_id();
            }
            else if (exp.check_type() == "CLAIM_SENTENCE") {
                EXPECT_EQ(getClaim(collection, exp.claim_id()).sentence(), exp.expected_claim_sentence())
                    << "[" << scenario.name() << "] CLAIM_SENTENCE " << exp.claim_id();
            }
            else if (exp.check_type() == "USER_VIEW") {
                auto actual = getUserView(collection, exp.claim_id(), username);
                auto expected = parseClaimStatus(exp.expected_status());
                EXPECT_EQ(actual, expected)
                    << "[" << scenario.name() << "] USER_VIEW " << exp.claim_id() << " " << username;
            }
            else if (exp.check_type() == "LINK_COUNT") {
                auto lt = parseLinkType(exp.link_type());
                EXPECT_EQ(countLinks(collection, lt), exp.expected_count())
                    << "[" << scenario.name() << "] LINK_COUNT " << exp.link_type();
            }
            else if (exp.check_type() == "LINK_EXISTS") {
                auto lt = parseLinkType(exp.link_type());
                EXPECT_TRUE(linkExists(collection, lt, exp.from_claim_id(), exp.to_claim_id()))
                    << "[" << scenario.name() << "] LINK_EXISTS "
                    << exp.from_claim_id() << " -> " << exp.to_claim_id();
            }
            else if (exp.check_type() == "COLLECTION_SIZE") {
                EXPECT_EQ(collection.claims_by_id_size(), exp.expected_count())
                    << "[" << scenario.name() << "] COLLECTION_SIZE";
            }
        }
    }


    // -----------------------------------------------------------------------
    // SECTION 7: FIXTURE MEMBER VARIABLES
    // -----------------------------------------------------------------------

    std::string db_path_;
    DebateModerator* moderator_ = nullptr;
    std::map<std::string, int> user_ids_;


    // -----------------------------------------------------------------------
    // SECTION 7a: LOAD SCENARIO FROM PBTXT FILE
    // -----------------------------------------------------------------------
    // Loads a TestScenario protobuf from a .pbtxt file on disk.
    // path: relative to the build directory (e.g. "scenarios/CreateDebate.pbtxt")

    TestScenario LoadScenarioFromFile(const std::string& path) {
        TestScenario scenario;
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "[LoadScenarioFromFile] FAILED to open: " << path << std::endl;
            return scenario;
        }
        std::string content((std::istreambuf_iterator<char>(file)),
                             std::istreambuf_iterator<char>());
        google::protobuf::TextFormat::ParseFromString(content, &scenario);
        return scenario;
    }
};


// ===========================================================================
// SECTION 8: TEST CASES
// ===========================================================================
// Each TEST_F loads a TestScenario from a .pbtxt file and calls executeScenario().
// Claim IDs are predictable: SQLite auto-increment gives root=1, first child=2,
// first challenge=3, etc.


// ---------------------------------------------------------------------------
// CreateDebate
// ---------------------------------------------------------------------------
// A creates a debate and enters it. Verifies root claim exists, sentence is
// correct, and A sees it as TRUE_CLAIM while B/C see UNDETERMINED.

TEST_F(ScenarioRunner, CreateDebate) {
    TestScenario s = LoadScenarioFromFile("scenarios/CreateDebate.pbtxt");
    ASSERT_GT(s.actions_size(), 0) << "Failed to load CreateDebate.pbtxt";
    executeScenario(s);

    // Dump step view layout as .pbtxt for inspection
    downloadPbtxt(1, "A", "step_view_create_debate.pbtxt");
}


// ---------------------------------------------------------------------------
// CreateDebate_FromPbtxt
// ---------------------------------------------------------------------------
// Duplicate of CreateDebate that also loads from pbtxt — kept as proof that
// the pbtxt loading pipeline works identically to inline construction.

TEST_F(ScenarioRunner, CreateDebate_FromPbtxt) {
    TestScenario s = LoadScenarioFromFile("scenarios/CreateDebate.pbtxt");
    ASSERT_GT(s.actions_size(), 0) << "Failed to load CreateDebate.pbtxt";
    executeScenario(s);
}


// ---------------------------------------------------------------------------
// EnterDebate
// ---------------------------------------------------------------------------
// A creates debate, all users enter. Verifies all are in ACTION_DEBATING state
// and all can see the root claim.

TEST_F(ScenarioRunner, EnterDebate) {
    TestScenario s = LoadScenarioFromFile("scenarios/EnterDebate.pbtxt");
    ASSERT_GT(s.actions_size(), 0) << "Failed to load EnterDebate.pbtxt";
    executeScenario(s);
}


// ---------------------------------------------------------------------------
// AddChildClaim
// ---------------------------------------------------------------------------
// A creates debate, all enter, A adds a child claim. Verifies child exists,
// sentence is correct, A sees it as TRUE_CLAIM, B/C see UNDETERMINED.
// Also verifies 1 PARENT_CHILD link.

TEST_F(ScenarioRunner, AddChildClaim) {
    TestScenario s = LoadScenarioFromFile("scenarios/AddChildClaim.pbtxt");
    ASSERT_GT(s.actions_size(), 0) << "Failed to load AddChildClaim.pbtxt";
    executeScenario(s);
}


// ---------------------------------------------------------------------------
// ChallengeClaim
// ---------------------------------------------------------------------------
// A creates debate + child, B challenges the child. Verifies challenge claim
// exists, per-user views, link counts, and engagement states.

TEST_F(ScenarioRunner, ChallengeClaim) {
    TestScenario s = LoadScenarioFromFile("scenarios/ChallengeClaim.pbtxt");
    ASSERT_GT(s.actions_size(), 0) << "Failed to load ChallengeClaim.pbtxt";
    executeScenario(s);
}


// ---------------------------------------------------------------------------
// FullStateVerification
// ---------------------------------------------------------------------------
// Comprehensive test: create debate, all join, add child, challenge.
// Verifies all claims, all per-user views, link counts, engagement states.

TEST_F(ScenarioRunner, FullStateVerification) {
    TestScenario s = LoadScenarioFromFile("scenarios/FullStateVerification.pbtxt");
    ASSERT_GT(s.actions_size(), 0) << "Failed to load FullStateVerification.pbtxt";
    executeScenario(s);
}


// ---------------------------------------------------------------------------
// CounterChallenge
// ---------------------------------------------------------------------------
// B challenges A's child, A counter-challenges B's challenge claim,
// B launches a second challenge on the same child.
// Verifies: counter-challenge exists, per-user views, 3 CHALLENGE links.

TEST_F(ScenarioRunner, CounterChallenge) {
    TestScenario s = LoadScenarioFromFile("scenarios/CounterChallenge.pbtxt");
    ASSERT_GT(s.actions_size(), 0) << "Failed to load CounterChallenge.pbtxt";
    executeScenario(s);
}


// ---------------------------------------------------------------------------
// ConcedeChallenge
// ---------------------------------------------------------------------------
// B challenges, A counter-challenges, B concedes.
// Currently ConcedeChallenge is a stub — only clears UI state.
// This test documents expected behavior for future implementation.

TEST_F(ScenarioRunner, ConcedeChallenge) {
    TestScenario s = LoadScenarioFromFile("scenarios/ConcedeChallenge.pbtxt");
    ASSERT_GT(s.actions_size(), 0) << "Failed to load ConcedeChallenge.pbtxt";
    executeScenario(s);
}


// ---------------------------------------------------------------------------
// FullLifecycle
// ---------------------------------------------------------------------------
// Complete flow: create, add child, challenge, counter-challenge, second
// challenge, concede. Verifies final state with all claims and engagement.

TEST_F(ScenarioRunner, FullLifecycle) {
    TestScenario s = LoadScenarioFromFile("scenarios/FullLifecycle.pbtxt");
    ASSERT_GT(s.actions_size(), 0) << "Failed to load FullLifecycle.pbtxt";
    executeScenario(s);

    // Dump step view layout as .pbtxt for inspection
    downloadPbtxt(1, "A", "step_view_full_lifecycle.pbtxt");
}
