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
// 4. executeScenario() runs the actions, collects responses, checks expectations
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
// CLAIM_EXISTS      — Claim ID is present in user's response collection
// CLAIM_SENTENCE    — Claim's text matches expected string
// USER_VIEW         — Per-user status on a claim (TRUE/FALSE/UNDETERMINED)
// LINK_COUNT        — Number of links of a given type in collection
// LINK_EXISTS       — Specific link from→to with given type exists
// ENGAGEMENT_STATE  — User's current action (ACTION_DEBATING / ACTION_HOME)
// COLLECTION_SIZE   — Total number of claims in user's collection
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
    // These are used by both forgeEvent() and executeScenario().

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
    // Sets user info and routes payload fields based on event type.

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
                // No payload — just opens the UI panel
                break;
            case debate_event::ADD_CHILD_CLAIM:
                event.mutable_add_child_claim()->set_claim(action.claim_text());
                event.mutable_add_child_claim()->set_description(
                    action.claim_description().empty() ? "test" : action.claim_description());
                break;
            case debate_event::SUBMIT_CHALLENGE_CLAIM:
                event.mutable_submit_challenge_claim()->set_challenge_sentence(action.claim_text());
                break;
            default:
                // No payload for other event types
                break;
        }
        return event;
    }


    // -----------------------------------------------------------------------
    // SECTION 4: COLLECTION BUILDER
    // -----------------------------------------------------------------------
    // Instead of sending a NONE event to each user (which is wasteful and
    // doesn't match how the production frontend works), we call
    // BuildCollection::BuildForDebateAndUsers() directly. This is the same
    // function the backend uses to build the response for the virtual renderer.
    //
    // It takes a debate_id and all user IDs, and returns ONE Collection
    // protobuf containing:
    //   - All claims in the debate (with per-user statuses in user_statuses map)
    //   - All links (PARENT_CHILD, CHALLENGE, etc.)
    //
    // This is more correct than per-user queries because:
    //   1. It tests the actual BuildCollection code path used in production
    //   2. It's a single call — no need to send N NONE events
    //   3. The collection has all user statuses on each claim in one place

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
    // Each claim in the collection has a user_statuses map that records every
    // user's opinion: TRUE_CLAIM, FALSE_CLAIM, or UNDETERMINED.

    // Look up a claim by ID in the collection
    debate::Claim getClaim(const debate::Collection& collection, int claim_id) {
        auto it = collection.claims_by_id().find(claim_id);
        if (it != collection.claims_by_id().end()) return it->second;
        return debate::Claim();  // empty claim (id == 0) if not found
    }

    // Get one user's status on a specific claim from the collection
    debate::ClaimStatus getUserView(const debate::Collection& collection,
                                     int claim_id, const std::string& username) {
        debate::Claim c = getClaim(collection, claim_id);
        if (c.id() == 0) return debate::ClaimStatus::UNDETERMINED;
        auto it = c.user_statuses().find(username);
        if (it != c.user_statuses().end()) return it->second;
        return debate::ClaimStatus::UNDETERMINED;
    }

    // Count all links of a given type in the collection
    int countLinks(const debate::Collection& collection, debate::LinkType type) {
        int count = 0;
        for (const auto& entry : collection.links_by_id()) {
            if (entry.second.link_type() == type) count++;
        }
        return count;
    }

    // Check if a specific link exists in the collection
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


    // -----------------------------------------------------------------------
    // SECTION 6: SCENARIO EXECUTOR
    // -----------------------------------------------------------------------
    // executeScenario: Runs a TestScenario end-to-end.
    //
    // Phase 1 — Execute actions:
    //   CREATE_USER → calls getUserId() (creates user in DB)
    //   All others → forgeEvent() → handleRequest()
    //   We also store each user's last response so we can check engagement state.
    //
    // Phase 2 — Build collection:
    //   Single call to BuildCollection::BuildForDebateAndUsers() with all user IDs.
    //   This gives us one Collection with all claims, links, and per-user statuses.
    //
    // Phase 3 — Check expectations:
    //   CLAIM_EXISTS, CLAIM_SENTENCE, USER_VIEW, LINK_COUNT, LINK_EXISTS, COLLECTION_SIZE
    //     → checked against the shared collection
    //   ENGAGEMENT_STATE
    //     → checked against the stored per-user response from Phase 1

    void executeScenario(const TestScenario& scenario) {

        // Phase 1: Execute all actions in order, storing each user's last response
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

        // Phase 2: Build a single collection for all users
        // We need a debate_id — find it from any JOIN_DEBATE or CREATE_DEBATE action
        int debate_id = 0;
        for (const auto& action : scenario.actions()) {
            if (action.event_type() == "CREATE_DEBATE") {
                // The first claim created is the root, which has the debate_id
                // We can get it from the DB, but simpler: use the first ENTER/JOIN debate_id
            }
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

        // Phase 3: Verify all expectations
        for (const auto& exp : scenario.expectations()) {
            const std::string& username = exp.username();

            if (exp.check_type() == "ENGAGEMENT_STATE") {
                // Engagement state is per-user, check from stored response
                auto resp_it = last_responses.find(username);
                ASSERT_TRUE(resp_it != last_responses.end())
                    << "[" << scenario.name() << "] ENGAGEMENT_STATE references unknown user: " << username;
                user_engagement::EngagementAction actual = resp_it->second.user().engagement().current_action();
                user_engagement::EngagementAction expected = parseEngagementAction(exp.expected_action());
                EXPECT_EQ(actual, expected)
                    << "[" << scenario.name() << "] ENGAGEMENT_STATE for user " << username
                    << " expected=" << exp.expected_action();
                continue;
            }

            // All other checks use the shared collection
            if (exp.check_type() == "CLAIM_EXISTS") {
                debate::Claim c = getClaim(collection, exp.claim_id());
                EXPECT_GT(c.id(), 0)
                    << "[" << scenario.name() << "] CLAIM_EXISTS claim_id=" << exp.claim_id()
                    << " for user " << username;
            }
            else if (exp.check_type() == "CLAIM_SENTENCE") {
                debate::Claim c = getClaim(collection, exp.claim_id());
                EXPECT_EQ(c.sentence(), exp.expected_claim_sentence())
                    << "[" << scenario.name() << "] CLAIM_SENTENCE claim_id=" << exp.claim_id()
                    << " for user " << username;
            }
            else if (exp.check_type() == "USER_VIEW") {
                debate::ClaimStatus actual = getUserView(collection, exp.claim_id(), username);
                debate::ClaimStatus expected = parseClaimStatus(exp.expected_status());
                EXPECT_EQ(actual, expected)
                    << "[" << scenario.name() << "] USER_VIEW claim_id=" << exp.claim_id()
                    << " user=" << username
                    << " expected=" << exp.expected_status();
            }
            else if (exp.check_type() == "LINK_COUNT") {
                debate::LinkType lt = parseLinkType(exp.link_type());
                int actual = countLinks(collection, lt);
                EXPECT_EQ(actual, exp.expected_count())
                    << "[" << scenario.name() << "] LINK_COUNT type=" << exp.link_type()
                    << " for user " << username;
            }
            else if (exp.check_type() == "LINK_EXISTS") {
                bool found = linkExists(collection, parseLinkType(exp.link_type()),
                                        exp.from_claim_id(), exp.to_claim_id());
                EXPECT_TRUE(found)
                    << "[" << scenario.name() << "] LINK_EXISTS "
                    << exp.from_claim_id() << " -> " << exp.to_claim_id()
                    << " type=" << exp.link_type() << " for user " << username;
            }
            else if (exp.check_type() == "COLLECTION_SIZE") {
                int actual = collection.claims_by_id_size();
                EXPECT_EQ(actual, exp.expected_count())
                    << "[" << scenario.name() << "] COLLECTION_SIZE for user " << username;
            }
        }
    }


    // -----------------------------------------------------------------------
    // SECTION 7: FIXTURE MEMBER VARIABLES
    // -----------------------------------------------------------------------
    std::string db_path_;
    DebateModerator* moderator_ = nullptr;
    std::map<std::string, int> user_ids_;  // username → user_id cache
};


// ===========================================================================
// SECTION 7: TEST CASES
// ===========================================================================
// Each TEST_F builds a TestScenario inline and calls executeScenario().
// The scenario defines ALL state — users, actions, and expected outcomes.
//
// To add a new test: copy the pattern below, set scenario.name, add actions
// (starting with CREATE_USER for each participant), then add expectations.


// ---------------------------------------------------------------------------
// ChallengeClaim
// ---------------------------------------------------------------------------
// A creates a debate with a child claim. B challenges the child.
// Verifies: claim exists, sentence correct, per-user statuses, link counts,
//           engagement states.
//
// Flow: CREATE_USER(A) → CREATE_USER(B) → CREATE_DEBATE → ENTER+JOIN(A,B)
//       → GO_TO_CLAIM(1) → OPEN_ADD_CHILD → ADD_CHILD_CLAIM
//       → GO_TO_CLAIM(2) → SUBMIT_CHALLENGE_CLAIM

TEST_F(ScenarioRunner, ChallengeClaim) {
    TestScenario scenario;
    scenario.set_name("ChallengeClaim");

    // --- Users ---
    auto* cu_a = scenario.add_actions();
    cu_a->set_username("A");
    cu_a->set_event_type("CREATE_USER");
    auto* cu_b = scenario.add_actions();
    cu_b->set_username("B");
    cu_b->set_event_type("CREATE_USER");

    // --- A creates debate and joins ---
    auto* a1 = scenario.add_actions();
    a1->set_username("A");
    a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    auto* a2 = scenario.add_actions();
    a2->set_username("A");
    a2->set_event_type("ENTER_DEBATE");
    a2->set_debate_id(1);
    auto* a3 = scenario.add_actions();
    a3->set_username("A");
    a3->set_event_type("JOIN_DEBATE");
    a3->set_debate_id(1);

    // --- B joins ---
    auto* b1 = scenario.add_actions();
    b1->set_username("B");
    b1->set_event_type("ENTER_DEBATE");
    b1->set_debate_id(1);
    auto* b2 = scenario.add_actions();
    b2->set_username("B");
    b2->set_event_type("JOIN_DEBATE");
    b2->set_debate_id(1);

    // --- A adds child claim under root (claim 1) ---
    auto* a4 = scenario.add_actions();
    a4->set_username("A");
    a4->set_event_type("GO_TO_CLAIM");
    a4->set_claim_id(1);
    auto* a5 = scenario.add_actions();
    a5->set_username("A");
    a5->set_event_type("OPEN_ADD_CHILD_CLAIM");
    auto* a6 = scenario.add_actions();
    a6->set_username("A");
    a6->set_event_type("ADD_CHILD_CLAIM");
    a6->set_claim_text("AI improves healthcare");

    // --- B navigates to child (claim 2) and challenges it ---
    auto* b3 = scenario.add_actions();
    b3->set_username("B");
    b3->set_event_type("GO_TO_CLAIM");
    b3->set_claim_id(2);
    auto* b4 = scenario.add_actions();
    b4->set_username("B");
    b4->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    b4->set_claim_text("Healthcare AI has bias issues");

    // --- Expectations ---

    auto* e1 = scenario.add_expectations();
    e1->set_check_type("CLAIM_EXISTS");
    e1->set_username("B");
    e1->set_claim_id(2);

    auto* e2 = scenario.add_expectations();
    e2->set_check_type("CLAIM_SENTENCE");
    e2->set_username("A");
    e2->set_claim_id(2);
    e2->set_expected_claim_sentence("AI improves healthcare");

    auto* e3 = scenario.add_expectations();
    e3->set_check_type("USER_VIEW");
    e3->set_username("A");
    e3->set_claim_id(2);
    e3->set_expected_status("TRUE_CLAIM");

    auto* e4 = scenario.add_expectations();
    e4->set_check_type("USER_VIEW");
    e4->set_username("B");
    e4->set_claim_id(2);
    e4->set_expected_status("FALSE_CLAIM");

    auto* e5 = scenario.add_expectations();
    e5->set_check_type("LINK_COUNT");
    e5->set_username("B");
    e5->set_link_type("PARENT_CHILD");
    e5->set_expected_count(1);
    auto* e6 = scenario.add_expectations();
    e6->set_check_type("LINK_COUNT");
    e6->set_username("B");
    e6->set_link_type("CHALLENGE");
    e6->set_expected_count(1);

    auto* e7 = scenario.add_expectations();
    e7->set_check_type("ENGAGEMENT_STATE");
    e7->set_username("A");
    e7->set_expected_action("ACTION_DEBATING");
    auto* e8 = scenario.add_expectations();
    e8->set_check_type("ENGAGEMENT_STATE");
    e8->set_username("B");
    e8->set_expected_action("ACTION_DEBATING");

    executeScenario(scenario);
}
