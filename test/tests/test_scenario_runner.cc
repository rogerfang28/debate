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
};


// ===========================================================================
// SECTION 8: TEST CASES
// ===========================================================================
// Each TEST_F builds a TestScenario inline and calls executeScenario().
// Claim IDs are predictable: SQLite auto-increment gives root=1, first child=2,
// first challenge=3, etc.


// ---------------------------------------------------------------------------
// CreateDebate
// ---------------------------------------------------------------------------
// A creates a debate and enters it. Verifies root claim exists, sentence is
// correct, and A sees it as TRUE_CLAIM while B/C see UNDETERMINED.

TEST_F(ScenarioRunner, CreateDebate) {
    TestScenario s;
    s.set_name("CreateDebate");

    auto* cu_a = s.add_actions(); cu_a->set_username("A"); cu_a->set_event_type("CREATE_USER");
    auto* cu_b = s.add_actions(); cu_b->set_username("B"); cu_b->set_event_type("CREATE_USER");
    auto* cu_c = s.add_actions(); cu_c->set_username("C"); cu_c->set_event_type("CREATE_USER");

    auto* a1 = s.add_actions();
    a1->set_username("A"); a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    auto* a2 = s.add_actions();
    a2->set_username("A"); a2->set_event_type("ENTER_DEBATE"); a2->set_debate_id(1);

    // Root claim exists
    auto* e1 = s.add_expectations();
    e1->set_check_type("CLAIM_EXISTS"); e1->set_username("A"); e1->set_claim_id(1);

    // Root sentence
    auto* e2 = s.add_expectations();
    e2->set_check_type("CLAIM_SENTENCE"); e2->set_username("A"); e2->set_claim_id(1);
    e2->set_expected_claim_sentence("Is AI beneficial?");

    // A sees root as TRUE_CLAIM
    auto* e3 = s.add_expectations();
    e3->set_check_type("USER_VIEW"); e3->set_username("A"); e3->set_claim_id(1);
    e3->set_expected_status("TRUE_CLAIM");

    // B and C see root as UNDETERMINED (not in debate yet)
    auto* e4 = s.add_expectations();
    e4->set_check_type("USER_VIEW"); e4->set_username("B"); e4->set_claim_id(1);
    e4->set_expected_status("UNDETERMINED");
    auto* e5 = s.add_expectations();
    e5->set_check_type("USER_VIEW"); e5->set_username("C"); e5->set_claim_id(1);
    e5->set_expected_status("UNDETERMINED");

    executeScenario(s);
}


// ---------------------------------------------------------------------------
// EnterDebate
// ---------------------------------------------------------------------------
// A creates debate, all users enter. Verifies all are in ACTION_DEBATING state
// and all can see the root claim.

TEST_F(ScenarioRunner, EnterDebate) {
    TestScenario s;
    s.set_name("EnterDebate");

    auto* cu_a = s.add_actions(); cu_a->set_username("A"); cu_a->set_event_type("CREATE_USER");
    auto* cu_b = s.add_actions(); cu_b->set_username("B"); cu_b->set_event_type("CREATE_USER");
    auto* cu_c = s.add_actions(); cu_c->set_username("C"); cu_c->set_event_type("CREATE_USER");

    auto* a1 = s.add_actions();
    a1->set_username("A"); a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    auto* a2 = s.add_actions();
    a2->set_username("A"); a2->set_event_type("ENTER_DEBATE"); a2->set_debate_id(1);
    auto* b1 = s.add_actions();
    b1->set_username("B"); b1->set_event_type("ENTER_DEBATE"); b1->set_debate_id(1);
    auto* c1 = s.add_actions();
    c1->set_username("C"); c1->set_event_type("ENTER_DEBATE"); c1->set_debate_id(1);

    // All users in debating state
    auto* e1 = s.add_expectations();
    e1->set_check_type("ENGAGEMENT_STATE"); e1->set_username("A");
    e1->set_expected_action("ACTION_DEBATING");
    auto* e2 = s.add_expectations();
    e2->set_check_type("ENGAGEMENT_STATE"); e2->set_username("B");
    e2->set_expected_action("ACTION_DEBATING");
    auto* e3 = s.add_expectations();
    e3->set_check_type("ENGAGEMENT_STATE"); e3->set_username("C");
    e3->set_expected_action("ACTION_DEBATING");

    // Root claim visible to all
    auto* e4 = s.add_expectations();
    e4->set_check_type("CLAIM_EXISTS"); e4->set_username("A"); e4->set_claim_id(1);
    auto* e5 = s.add_expectations();
    e5->set_check_type("CLAIM_SENTENCE"); e5->set_username("A"); e5->set_claim_id(1);
    e5->set_expected_claim_sentence("Is AI beneficial?");

    executeScenario(s);
}


// ---------------------------------------------------------------------------
// AddChildClaim
// ---------------------------------------------------------------------------
// A creates debate, all enter, A adds a child claim. Verifies child exists,
// sentence is correct, A sees it as TRUE_CLAIM, B/C see UNDETERMINED.
// Also verifies 1 PARENT_CHILD link.

TEST_F(ScenarioRunner, AddChildClaim) {
    TestScenario s;
    s.set_name("AddChildClaim");

    auto* cu_a = s.add_actions(); cu_a->set_username("A"); cu_a->set_event_type("CREATE_USER");
    auto* cu_b = s.add_actions(); cu_b->set_username("B"); cu_b->set_event_type("CREATE_USER");
    auto* cu_c = s.add_actions(); cu_c->set_username("C"); cu_c->set_event_type("CREATE_USER");

    auto* a1 = s.add_actions();
    a1->set_username("A"); a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    auto* a2 = s.add_actions();
    a2->set_username("A"); a2->set_event_type("ENTER_DEBATE"); a2->set_debate_id(1);
    auto* b1 = s.add_actions();
    b1->set_username("B"); b1->set_event_type("ENTER_DEBATE"); b1->set_debate_id(1);
    auto* c1 = s.add_actions();
    c1->set_username("C"); c1->set_event_type("ENTER_DEBATE"); c1->set_debate_id(1);

    // A adds child under root (claim 1)
    auto* a3 = s.add_actions();
    a3->set_username("A"); a3->set_event_type("GO_TO_CLAIM"); a3->set_claim_id(1);
    auto* a4 = s.add_actions();
    a4->set_username("A"); a4->set_event_type("OPEN_ADD_CHILD_CLAIM");
    auto* a5 = s.add_actions();
    a5->set_username("A"); a5->set_event_type("ADD_CHILD_CLAIM");
    a5->set_claim_text("AI improves healthcare");

    // Child claim (ID 2) exists
    auto* e1 = s.add_expectations();
    e1->set_check_type("CLAIM_EXISTS"); e1->set_username("A"); e1->set_claim_id(2);

    // Child sentence
    auto* e2 = s.add_expectations();
    e2->set_check_type("CLAIM_SENTENCE"); e2->set_username("A"); e2->set_claim_id(2);
    e2->set_expected_claim_sentence("AI improves healthcare");

    // A sees child as TRUE_CLAIM
    auto* e3 = s.add_expectations();
    e3->set_check_type("USER_VIEW"); e3->set_username("A"); e3->set_claim_id(2);
    e3->set_expected_status("TRUE_CLAIM");

    // B/C see child as UNDETERMINED
    auto* e4 = s.add_expectations();
    e4->set_check_type("USER_VIEW"); e4->set_username("B"); e4->set_claim_id(2);
    e4->set_expected_status("UNDETERMINED");

    // 1 PARENT_CHILD link
    auto* e5 = s.add_expectations();
    e5->set_check_type("LINK_COUNT"); e5->set_username("A");
    e5->set_link_type("PARENT_CHILD"); e5->set_expected_count(1);

    executeScenario(s);
}


// ---------------------------------------------------------------------------
// ChallengeClaim
// ---------------------------------------------------------------------------
// A creates debate + child, B challenges the child. Verifies challenge claim
// exists, per-user views, link counts, and engagement states.

TEST_F(ScenarioRunner, ChallengeClaim) {
    TestScenario s;
    s.set_name("ChallengeClaim");

    auto* cu_a = s.add_actions(); cu_a->set_username("A"); cu_a->set_event_type("CREATE_USER");
    auto* cu_b = s.add_actions(); cu_b->set_username("B"); cu_b->set_event_type("CREATE_USER");
    auto* cu_c = s.add_actions(); cu_c->set_username("C"); cu_c->set_event_type("CREATE_USER");

    auto* a1 = s.add_actions();
    a1->set_username("A"); a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    // All enter + join (JOIN needed for collection visibility)
    auto* a2 = s.add_actions();
    a2->set_username("A"); a2->set_event_type("ENTER_DEBATE"); a2->set_debate_id(1);
    auto* a3 = s.add_actions();
    a3->set_username("A"); a3->set_event_type("JOIN_DEBATE"); a3->set_debate_id(1);
    auto* b1 = s.add_actions();
    b1->set_username("B"); b1->set_event_type("ENTER_DEBATE"); b1->set_debate_id(1);
    auto* b2 = s.add_actions();
    b2->set_username("B"); b2->set_event_type("JOIN_DEBATE"); b2->set_debate_id(1);
    auto* c1 = s.add_actions();
    c1->set_username("C"); c1->set_event_type("ENTER_DEBATE"); c1->set_debate_id(1);
    auto* c2 = s.add_actions();
    c2->set_username("C"); c2->set_event_type("JOIN_DEBATE"); c2->set_debate_id(1);

    // A adds child (claim 2)
    auto* a4 = s.add_actions();
    a4->set_username("A"); a4->set_event_type("GO_TO_CLAIM"); a4->set_claim_id(1);
    auto* a5 = s.add_actions();
    a5->set_username("A"); a5->set_event_type("OPEN_ADD_CHILD_CLAIM");
    auto* a6 = s.add_actions();
    a6->set_username("A"); a6->set_event_type("ADD_CHILD_CLAIM");
    a6->set_claim_text("AI improves healthcare");

    // B challenges child (claim 2) → creates challenge claim (ID 3)
    auto* b3 = s.add_actions();
    b3->set_username("B"); b3->set_event_type("GO_TO_CLAIM"); b3->set_claim_id(2);
    auto* b4 = s.add_actions();
    b4->set_username("B"); b4->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    b4->set_claim_text("Healthcare AI has bias issues");

    // Child claim exists
    auto* e1 = s.add_expectations();
    e1->set_check_type("CLAIM_EXISTS"); e1->set_username("B"); e1->set_claim_id(2);

    // Challenge claim (ID 3) exists
    auto* e2 = s.add_expectations();
    e2->set_check_type("CLAIM_EXISTS"); e2->set_username("B"); e2->set_claim_id(3);

    // Challenge sentence
    auto* e3 = s.add_expectations();
    e3->set_check_type("CLAIM_SENTENCE"); e3->set_username("B"); e3->set_claim_id(3);
    e3->set_expected_claim_sentence("Healthcare AI has bias issues");

    // A sees child as TRUE_CLAIM
    auto* e4 = s.add_expectations();
    e4->set_check_type("USER_VIEW"); e4->set_username("A"); e4->set_claim_id(2);
    e4->set_expected_status("TRUE_CLAIM");

    // B sees child as FALSE_CLAIM
    auto* e5 = s.add_expectations();
    e5->set_check_type("USER_VIEW"); e5->set_username("B"); e5->set_claim_id(2);
    e5->set_expected_status("FALSE_CLAIM");

    // C sees child as UNDETERMINED
    auto* e6 = s.add_expectations();
    e6->set_check_type("USER_VIEW"); e6->set_username("C"); e6->set_claim_id(2);
    e6->set_expected_status("UNDETERMINED");

    // B sees own challenge as TRUE_CLAIM
    auto* e7 = s.add_expectations();
    e7->set_check_type("USER_VIEW"); e7->set_username("B"); e7->set_claim_id(3);
    e7->set_expected_status("TRUE_CLAIM");

    // A sees challenge as UNDETERMINED
    auto* e8 = s.add_expectations();
    e8->set_check_type("USER_VIEW"); e8->set_username("A"); e8->set_claim_id(3);
    e8->set_expected_status("UNDETERMINED");

    // 1 PARENT_CHILD, 1 CHALLENGE link
    auto* e9 = s.add_expectations();
    e9->set_check_type("LINK_COUNT"); e9->set_username("B");
    e9->set_link_type("PARENT_CHILD"); e9->set_expected_count(1);
    auto* e10 = s.add_expectations();
    e10->set_check_type("LINK_COUNT"); e10->set_username("B");
    e10->set_link_type("CHALLENGE"); e10->set_expected_count(1);

    // All in debating state
    auto* e11 = s.add_expectations();
    e11->set_check_type("ENGAGEMENT_STATE"); e11->set_username("A");
    e11->set_expected_action("ACTION_DEBATING");
    auto* e12 = s.add_expectations();
    e12->set_check_type("ENGAGEMENT_STATE"); e12->set_username("B");
    e12->set_expected_action("ACTION_DEBATING");

    executeScenario(s);
}


// ---------------------------------------------------------------------------
// FullStateVerification
// ---------------------------------------------------------------------------
// Comprehensive test: create debate, all join, add child, challenge.
// Verifies all claims, all per-user views, link counts, engagement states.

TEST_F(ScenarioRunner, FullStateVerification) {
    TestScenario s;
    s.set_name("FullStateVerification");

    auto* cu_a = s.add_actions(); cu_a->set_username("A"); cu_a->set_event_type("CREATE_USER");
    auto* cu_b = s.add_actions(); cu_b->set_username("B"); cu_b->set_event_type("CREATE_USER");
    auto* cu_c = s.add_actions(); cu_c->set_username("C"); cu_c->set_event_type("CREATE_USER");

    auto* a1 = s.add_actions();
    a1->set_username("A"); a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    // All enter + join
    auto* a2 = s.add_actions();
    a2->set_username("A"); a2->set_event_type("ENTER_DEBATE"); a2->set_debate_id(1);
    auto* a3 = s.add_actions();
    a3->set_username("A"); a3->set_event_type("JOIN_DEBATE"); a3->set_debate_id(1);
    auto* b1 = s.add_actions();
    b1->set_username("B"); b1->set_event_type("ENTER_DEBATE"); b1->set_debate_id(1);
    auto* b2 = s.add_actions();
    b2->set_username("B"); b2->set_event_type("JOIN_DEBATE"); b2->set_debate_id(1);
    auto* c1 = s.add_actions();
    c1->set_username("C"); c1->set_event_type("ENTER_DEBATE"); c1->set_debate_id(1);
    auto* c2 = s.add_actions();
    c2->set_username("C"); c2->set_event_type("JOIN_DEBATE"); c2->set_debate_id(1);

    // A adds child (claim 2)
    auto* a4 = s.add_actions();
    a4->set_username("A"); a4->set_event_type("GO_TO_CLAIM"); a4->set_claim_id(1);
    auto* a5 = s.add_actions();
    a5->set_username("A"); a5->set_event_type("OPEN_ADD_CHILD_CLAIM");
    auto* a6 = s.add_actions();
    a6->set_username("A"); a6->set_event_type("ADD_CHILD_CLAIM");
    a6->set_claim_text("AI improves healthcare");

    // B challenges child (claim 2) → challenge claim ID 3
    auto* b3 = s.add_actions();
    b3->set_username("B"); b3->set_event_type("GO_TO_CLAIM"); b3->set_claim_id(2);
    auto* b4 = s.add_actions();
    b4->set_username("B"); b4->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    b4->set_claim_text("Healthcare AI has bias issues");

    // --- Root claim (ID 1) ---
    auto* e1 = s.add_expectations();
    e1->set_check_type("CLAIM_EXISTS"); e1->set_username("A"); e1->set_claim_id(1);
    auto* e2 = s.add_expectations();
    e2->set_check_type("CLAIM_SENTENCE"); e2->set_username("A"); e2->set_claim_id(1);
    e2->set_expected_claim_sentence("Is AI beneficial?");

    // Root per-user views
    auto* e3 = s.add_expectations();
    e3->set_check_type("USER_VIEW"); e3->set_username("A"); e3->set_claim_id(1);
    e3->set_expected_status("TRUE_CLAIM");
    auto* e4 = s.add_expectations();
    e4->set_check_type("USER_VIEW"); e4->set_username("B"); e4->set_claim_id(1);
    e4->set_expected_status("UNDETERMINED");
    auto* e5 = s.add_expectations();
    e5->set_check_type("USER_VIEW"); e5->set_username("C"); e5->set_claim_id(1);
    e5->set_expected_status("UNDETERMINED");

    // --- Child claim (ID 2) ---
    auto* e6 = s.add_expectations();
    e6->set_check_type("CLAIM_EXISTS"); e6->set_username("A"); e6->set_claim_id(2);
    auto* e7 = s.add_expectations();
    e7->set_check_type("CLAIM_SENTENCE"); e7->set_username("A"); e7->set_claim_id(2);
    e7->set_expected_claim_sentence("AI improves healthcare");

    // Child per-user views
    auto* e8 = s.add_expectations();
    e8->set_check_type("USER_VIEW"); e8->set_username("A"); e8->set_claim_id(2);
    e8->set_expected_status("TRUE_CLAIM");
    auto* e9 = s.add_expectations();
    e9->set_check_type("USER_VIEW"); e9->set_username("B"); e9->set_claim_id(2);
    e9->set_expected_status("FALSE_CLAIM");
    auto* e10 = s.add_expectations();
    e10->set_check_type("USER_VIEW"); e10->set_username("C"); e10->set_claim_id(2);
    e10->set_expected_status("UNDETERMINED");

    // --- Challenge claim (ID 3) ---
    auto* e11 = s.add_expectations();
    e11->set_check_type("CLAIM_EXISTS"); e11->set_username("B"); e11->set_claim_id(3);
    auto* e12 = s.add_expectations();
    e12->set_check_type("CLAIM_SENTENCE"); e12->set_username("B"); e12->set_claim_id(3);
    e12->set_expected_claim_sentence("Healthcare AI has bias issues");

    // Challenge per-user views
    auto* e13 = s.add_expectations();
    e13->set_check_type("USER_VIEW"); e13->set_username("B"); e13->set_claim_id(3);
    e13->set_expected_status("TRUE_CLAIM");
    auto* e14 = s.add_expectations();
    e14->set_check_type("USER_VIEW"); e14->set_username("A"); e14->set_claim_id(3);
    e14->set_expected_status("UNDETERMINED");

    // Link counts
    auto* e15 = s.add_expectations();
    e15->set_check_type("LINK_COUNT"); e15->set_username("B");
    e15->set_link_type("PARENT_CHILD"); e15->set_expected_count(1);
    auto* e16 = s.add_expectations();
    e16->set_check_type("LINK_COUNT"); e16->set_username("B");
    e16->set_link_type("CHALLENGE"); e16->set_expected_count(1);

    // All in debating
    auto* e17 = s.add_expectations();
    e17->set_check_type("ENGAGEMENT_STATE"); e17->set_username("A");
    e17->set_expected_action("ACTION_DEBATING");
    auto* e18 = s.add_expectations();
    e18->set_check_type("ENGAGEMENT_STATE"); e18->set_username("B");
    e18->set_expected_action("ACTION_DEBATING");
    auto* e19 = s.add_expectations();
    e19->set_check_type("ENGAGEMENT_STATE"); e19->set_username("C");
    e19->set_expected_action("ACTION_DEBATING");

    executeScenario(s);
}


// ---------------------------------------------------------------------------
// CounterChallenge
// ---------------------------------------------------------------------------
// B challenges A's child, A counter-challenges B's challenge claim,
// B launches a second challenge on the same child.
// Verifies: counter-challenge exists, per-user views, 3 CHALLENGE links.

TEST_F(ScenarioRunner, CounterChallenge) {
    TestScenario s;
    s.set_name("CounterChallenge");

    auto* cu_a = s.add_actions(); cu_a->set_username("A"); cu_a->set_event_type("CREATE_USER");
    auto* cu_b = s.add_actions(); cu_b->set_username("B"); cu_b->set_event_type("CREATE_USER");
    auto* cu_c = s.add_actions(); cu_c->set_username("C"); cu_c->set_event_type("CREATE_USER");

    auto* a1 = s.add_actions();
    a1->set_username("A"); a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    // All enter + join
    auto* a2 = s.add_actions();
    a2->set_username("A"); a2->set_event_type("ENTER_DEBATE"); a2->set_debate_id(1);
    auto* a3 = s.add_actions();
    a3->set_username("A"); a3->set_event_type("JOIN_DEBATE"); a3->set_debate_id(1);
    auto* b1 = s.add_actions();
    b1->set_username("B"); b1->set_event_type("ENTER_DEBATE"); b1->set_debate_id(1);
    auto* b2 = s.add_actions();
    b2->set_username("B"); b2->set_event_type("JOIN_DEBATE"); b2->set_debate_id(1);
    auto* c1 = s.add_actions();
    c1->set_username("C"); c1->set_event_type("ENTER_DEBATE"); c1->set_debate_id(1);
    auto* c2 = s.add_actions();
    c2->set_username("C"); c2->set_event_type("JOIN_DEBATE"); c2->set_debate_id(1);

    // A adds child (claim 2)
    auto* a4 = s.add_actions();
    a4->set_username("A"); a4->set_event_type("GO_TO_CLAIM"); a4->set_claim_id(1);
    auto* a5 = s.add_actions();
    a5->set_username("A"); a5->set_event_type("OPEN_ADD_CHILD_CLAIM");
    auto* a6 = s.add_actions();
    a6->set_username("A"); a6->set_event_type("ADD_CHILD_CLAIM");
    a6->set_claim_text("AI improves healthcare");

    // B challenges child (claim 2) → challenge claim ID 3
    auto* b3 = s.add_actions();
    b3->set_username("B"); b3->set_event_type("GO_TO_CLAIM"); b3->set_claim_id(2);
    auto* b4 = s.add_actions();
    b4->set_username("B"); b4->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    b4->set_claim_text("Healthcare AI has bias issues");

    // A counter-challenges B's challenge claim (ID 3) → counter-challenge ID 4
    auto* a7 = s.add_actions();
    a7->set_username("A"); a7->set_event_type("GO_TO_CLAIM"); a7->set_claim_id(3);
    auto* a8 = s.add_actions();
    a8->set_username("A"); a8->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    a8->set_claim_text("Actually healthcare AI is beneficial");

    // B second challenge on child (claim 2) → challenge claim ID 5
    auto* b5 = s.add_actions();
    b5->set_username("B"); b5->set_event_type("GO_TO_CLAIM"); b5->set_claim_id(2);
    auto* b6 = s.add_actions();
    b6->set_username("B"); b6->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    b6->set_claim_text("Healthcare AI has privacy issues too");

    // Counter-challenge (ID 4) exists
    auto* e1 = s.add_expectations();
    e1->set_check_type("CLAIM_EXISTS"); e1->set_username("A"); e1->set_claim_id(4);

    // Counter-challenge sentence
    auto* e2 = s.add_expectations();
    e2->set_check_type("CLAIM_SENTENCE"); e2->set_username("A"); e2->set_claim_id(4);
    e2->set_expected_claim_sentence("Actually healthcare AI is beneficial");

    // A sees counter-challenge as TRUE_CLAIM
    auto* e3 = s.add_expectations();
    e3->set_check_type("USER_VIEW"); e3->set_username("A"); e3->set_claim_id(4);
    e3->set_expected_status("TRUE_CLAIM");

    // B sees counter-challenge as UNDETERMINED
    auto* e4 = s.add_expectations();
    e4->set_check_type("USER_VIEW"); e4->set_username("B"); e4->set_claim_id(4);
    e4->set_expected_status("UNDETERMINED");

    // A sees B's challenge (ID 3) as FALSE_CLAIM
    auto* e5 = s.add_expectations();
    e5->set_check_type("USER_VIEW"); e5->set_username("A"); e5->set_claim_id(3);
    e5->set_expected_status("FALSE_CLAIM");

    // B's second challenge (ID 5) exists
    auto* e6 = s.add_expectations();
    e6->set_check_type("CLAIM_EXISTS"); e6->set_username("B"); e6->set_claim_id(5);

    // Second challenge sentence
    auto* e7 = s.add_expectations();
    e7->set_check_type("CLAIM_SENTENCE"); e7->set_username("B"); e7->set_claim_id(5);
    e7->set_expected_claim_sentence("Healthcare AI has privacy issues too");

    // A still sees child as TRUE_CLAIM
    auto* e8 = s.add_expectations();
    e8->set_check_type("USER_VIEW"); e8->set_username("A"); e8->set_claim_id(2);
    e8->set_expected_status("TRUE_CLAIM");

    // B still sees child as FALSE_CLAIM
    auto* e9 = s.add_expectations();
    e9->set_check_type("USER_VIEW"); e9->set_username("B"); e9->set_claim_id(2);
    e9->set_expected_status("FALSE_CLAIM");

    // 3 CHALLENGE links total (B's first + A's counter + B's second)
    auto* e10 = s.add_expectations();
    e10->set_check_type("LINK_COUNT"); e10->set_username("B");
    e10->set_link_type("CHALLENGE"); e10->set_expected_count(3);

    executeScenario(s);
}


// ---------------------------------------------------------------------------
// ConcedeChallenge
// ---------------------------------------------------------------------------
// B challenges, A counter-challenges, B concedes.
// Currently ConcedeChallenge is a stub — only clears UI state.
// This test documents expected behavior for future implementation.

TEST_F(ScenarioRunner, ConcedeChallenge) {
    TestScenario s;
    s.set_name("ConcedeChallenge");

    auto* cu_a = s.add_actions(); cu_a->set_username("A"); cu_a->set_event_type("CREATE_USER");
    auto* cu_b = s.add_actions(); cu_b->set_username("B"); cu_b->set_event_type("CREATE_USER");
    auto* cu_c = s.add_actions(); cu_c->set_username("C"); cu_c->set_event_type("CREATE_USER");

    auto* a1 = s.add_actions();
    a1->set_username("A"); a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    // All enter + join
    auto* a2 = s.add_actions();
    a2->set_username("A"); a2->set_event_type("ENTER_DEBATE"); a2->set_debate_id(1);
    auto* a3 = s.add_actions();
    a3->set_username("A"); a3->set_event_type("JOIN_DEBATE"); a3->set_debate_id(1);
    auto* b1 = s.add_actions();
    b1->set_username("B"); b1->set_event_type("ENTER_DEBATE"); b1->set_debate_id(1);
    auto* b2 = s.add_actions();
    b2->set_username("B"); b2->set_event_type("JOIN_DEBATE"); b2->set_debate_id(1);
    auto* c1 = s.add_actions();
    c1->set_username("C"); c1->set_event_type("ENTER_DEBATE"); c1->set_debate_id(1);
    auto* c2 = s.add_actions();
    c2->set_username("C"); c2->set_event_type("JOIN_DEBATE"); c2->set_debate_id(1);

    // A adds child (claim 2)
    auto* a4 = s.add_actions();
    a4->set_username("A"); a4->set_event_type("GO_TO_CLAIM"); a4->set_claim_id(1);
    auto* a5 = s.add_actions();
    a5->set_username("A"); a5->set_event_type("OPEN_ADD_CHILD_CLAIM");
    auto* a6 = s.add_actions();
    a6->set_username("A"); a6->set_event_type("ADD_CHILD_CLAIM");
    a6->set_claim_text("AI improves healthcare");

    // B challenges child (claim 2) → challenge claim ID 3
    auto* b3 = s.add_actions();
    b3->set_username("B"); b3->set_event_type("GO_TO_CLAIM"); b3->set_claim_id(2);
    auto* b4 = s.add_actions();
    b4->set_username("B"); b4->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    b4->set_claim_text("Healthcare AI has bias issues");

    // A counter-challenges B's challenge (ID 3) → counter-challenge ID 4
    auto* a7 = s.add_actions();
    a7->set_username("A"); a7->set_event_type("GO_TO_CLAIM"); a7->set_claim_id(3);
    auto* a8 = s.add_actions();
    a8->set_username("A"); a8->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    a8->set_claim_text("Actually healthcare AI is beneficial");

    // Verify state before concede
    auto* e_pre1 = s.add_expectations();
    e_pre1->set_check_type("USER_VIEW"); e_pre1->set_username("A"); e_pre1->set_claim_id(4);
    e_pre1->set_expected_status("TRUE_CLAIM");
    auto* e_pre2 = s.add_expectations();
    e_pre2->set_check_type("USER_VIEW"); e_pre2->set_username("A"); e_pre2->set_claim_id(3);
    e_pre2->set_expected_status("FALSE_CLAIM");

    // B concedes
    auto* b5 = s.add_actions();
    b5->set_username("B"); b5->set_event_type("CONCEDE_CHALLENGE");

    // After concede: all claims still exist
    auto* e1 = s.add_expectations();
    e1->set_check_type("CLAIM_EXISTS"); e1->set_username("A"); e1->set_claim_id(1);
    auto* e2 = s.add_expectations();
    e2->set_check_type("CLAIM_EXISTS"); e2->set_username("A"); e2->set_claim_id(2);
    auto* e3 = s.add_expectations();
    e3->set_check_type("CLAIM_EXISTS"); e3->set_username("A"); e3->set_claim_id(3);
    auto* e4 = s.add_expectations();
    e4->set_check_type("CLAIM_EXISTS"); e4->set_username("A"); e4->set_claim_id(4);

    // All still in debating
    auto* e5 = s.add_expectations();
    e5->set_check_type("ENGAGEMENT_STATE"); e5->set_username("A");
    e5->set_expected_action("ACTION_DEBATING");
    auto* e6 = s.add_expectations();
    e6->set_check_type("ENGAGEMENT_STATE"); e6->set_username("B");
    e6->set_expected_action("ACTION_DEBATING");

    // TODO: After ConcedeChallenge is implemented, add:
    //   - B's challenge claim (3) → FALSE_CLAIM (conceded)
    //   - A's counter-challenge (4) → TRUE_CLAIM (upheld)
    //   - Challenge link removed

    executeScenario(s);
}


// ---------------------------------------------------------------------------
// FullLifecycle
// ---------------------------------------------------------------------------
// Complete flow: create, add child, challenge, counter-challenge, second
// challenge, concede. Verifies final state with all claims and engagement.

TEST_F(ScenarioRunner, FullLifecycle) {
    TestScenario s;
    s.set_name("FullLifecycle");

    auto* cu_a = s.add_actions(); cu_a->set_username("A"); cu_a->set_event_type("CREATE_USER");
    auto* cu_b = s.add_actions(); cu_b->set_username("B"); cu_b->set_event_type("CREATE_USER");
    auto* cu_c = s.add_actions(); cu_c->set_username("C"); cu_c->set_event_type("CREATE_USER");

    auto* a1 = s.add_actions();
    a1->set_username("A"); a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    // All enter + join
    auto* a2 = s.add_actions();
    a2->set_username("A"); a2->set_event_type("ENTER_DEBATE"); a2->set_debate_id(1);
    auto* a3 = s.add_actions();
    a3->set_username("A"); a3->set_event_type("JOIN_DEBATE"); a3->set_debate_id(1);
    auto* b1 = s.add_actions();
    b1->set_username("B"); b1->set_event_type("ENTER_DEBATE"); b1->set_debate_id(1);
    auto* b2 = s.add_actions();
    b2->set_username("B"); b2->set_event_type("JOIN_DEBATE"); b2->set_debate_id(1);
    auto* c1 = s.add_actions();
    c1->set_username("C"); c1->set_event_type("ENTER_DEBATE"); c1->set_debate_id(1);
    auto* c2 = s.add_actions();
    c2->set_username("C"); c2->set_event_type("JOIN_DEBATE"); c2->set_debate_id(1);

    // A adds child (claim 2)
    auto* a4 = s.add_actions();
    a4->set_username("A"); a4->set_event_type("GO_TO_CLAIM"); a4->set_claim_id(1);
    auto* a5 = s.add_actions();
    a5->set_username("A"); a5->set_event_type("OPEN_ADD_CHILD_CLAIM");
    auto* a6 = s.add_actions();
    a6->set_username("A"); a6->set_event_type("ADD_CHILD_CLAIM");
    a6->set_claim_text("AI improves healthcare");

    // B challenges child (claim 2) → challenge claim ID 3
    auto* b3 = s.add_actions();
    b3->set_username("B"); b3->set_event_type("GO_TO_CLAIM"); b3->set_claim_id(2);
    auto* b4 = s.add_actions();
    b4->set_username("B"); b4->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    b4->set_claim_text("Healthcare AI has bias issues");

    // A counter-challenges B's challenge (ID 3) → counter-challenge ID 4
    auto* a7 = s.add_actions();
    a7->set_username("A"); a7->set_event_type("GO_TO_CLAIM"); a7->set_claim_id(3);
    auto* a8 = s.add_actions();
    a8->set_username("A"); a8->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    a8->set_claim_text("Actually healthcare AI is beneficial");

    // B second challenge on child (claim 2) → challenge claim ID 5
    auto* b5 = s.add_actions();
    b5->set_username("B"); b5->set_event_type("GO_TO_CLAIM"); b5->set_claim_id(2);
    auto* b6 = s.add_actions();
    b6->set_username("B"); b6->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    b6->set_claim_text("Healthcare AI has privacy issues too");

    // B concedes
    auto* b7 = s.add_actions();
    b7->set_username("B"); b7->set_event_type("CONCEDE_CHALLENGE");

    // All claims still exist: root(1), child(2), B_challenge(3), A_counter(4), B_second(5)
    auto* e1 = s.add_expectations();
    e1->set_check_type("CLAIM_EXISTS"); e1->set_username("A"); e1->set_claim_id(1);
    auto* e2 = s.add_expectations();
    e2->set_check_type("CLAIM_EXISTS"); e2->set_username("A"); e2->set_claim_id(2);
    auto* e3 = s.add_expectations();
    e3->set_check_type("CLAIM_EXISTS"); e3->set_username("A"); e3->set_claim_id(3);
    auto* e4 = s.add_expectations();
    e4->set_check_type("CLAIM_EXISTS"); e4->set_username("A"); e4->set_claim_id(4);
    auto* e5 = s.add_expectations();
    e5->set_check_type("CLAIM_EXISTS"); e5->set_username("A"); e5->set_claim_id(5);

    // All still in debating
    auto* e6 = s.add_expectations();
    e6->set_check_type("ENGAGEMENT_STATE"); e6->set_username("A");
    e6->set_expected_action("ACTION_DEBATING");
    auto* e7 = s.add_expectations();
    e7->set_check_type("ENGAGEMENT_STATE"); e7->set_username("B");
    e7->set_expected_action("ACTION_DEBATING");
    auto* e8 = s.add_expectations();
    e8->set_check_type("ENGAGEMENT_STATE"); e8->set_username("C");
    e8->set_expected_action("ACTION_DEBATING");

    // 3 CHALLENGE links (B1 + A counter + B2)
    auto* e9 = s.add_expectations();
    e9->set_check_type("LINK_COUNT"); e9->set_username("B");
    e9->set_link_type("CHALLENGE"); e9->set_expected_count(3);

    // Collection has 5 claims total
    auto* e10 = s.add_expectations();
    e10->set_check_type("COLLECTION_SIZE"); e10->set_username("A");
    e10->set_expected_count(5);

    executeScenario(s);
}
