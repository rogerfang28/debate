// test_scenario_runner.cc — Data-driven test runner for TestScenario protobufs
//
// Loads TestScenario messages from test/scenarios/ directory and executes each
// one through the DebateModerator event pipeline. Each scenario defines a
// sequence of user actions and a set of expected outcomes.
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

using debate_test::TestScenario;
using debate_test::TestAction;
using debate_test::TestExpectation;

// -------------------------------------------------------
// Fixture
// -------------------------------------------------------
class ScenarioRunner : public ::testing::Test {
protected:
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

    // Resolve username to user_id, creating the user if needed
    int getUserId(const std::string& username) {
        auto it = user_ids_.find(username);
        if (it != user_ids_.end()) return it->second;
        int id = moderator_->createUserIfNotExist(username);
        user_ids_[username] = id;
        return id;
    }

    // Convert string event_type to enum
    debate_event::EventType parseEventType(const std::string& name) {
        debate_event::EventType type;
        if (debate_event::EventType_Parse(name, &type)) return type;
        // Fallback: try with full qualifier
        std::string full = "debate_event::" + name;
        if (debate_event::EventType_Parse(full, &type)) return type;
        return debate_event::NONE;
    }

    // Convert string status to ClaimStatus enum
    debate::ClaimStatus parseClaimStatus(const std::string& s) {
        if (s == "TRUE_CLAIM") return debate::ClaimStatus::TRUE_CLAIM;
        if (s == "FALSE_CLAIM") return debate::ClaimStatus::FALSE_CLAIM;
        return debate::ClaimStatus::UNDETERMINED;
    }

    // Convert string action to engagement enum
    user_engagement::EngagementAction parseUserAction(const std::string& s) {
        if (s == "ACTION_DEBATING") return user_engagement::ACTION_DEBATING;
        return user_engagement::ACTION_HOME;
    }

    // Convert string to LinkType
    debate::LinkType parseLinkType(const std::string& s) {
        if (s == "CHALLENGE") return debate::LinkType::CHALLENGE;
        return debate::LinkType::PARENT_CHILD;
    }

    // Forge a DebateEvent from a TestAction
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
                // No payload
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
                break;
        }
        return event;
    }

    // Send event and return the response
    moderator_to_vr::ModeratorToVRMessage sendEvent(int user_id, const debate_event::DebateEvent& event) {
        return moderator_->handleRequest(const_cast<debate_event::DebateEvent&>(event));
    }

    // Get claim from a user's response collection
    debate::Claim getClaimFromResponse(const moderator_to_vr::ModeratorToVRMessage& resp, int claim_id) {
        auto it = resp.collection().claims_by_id().find(claim_id);
        if (it != resp.collection().claims_by_id().end()) return it->second;
        return debate::Claim();
    }

    // Get per-user status on a claim from a response collection
    debate::ClaimStatus getUserView(const moderator_to_vr::ModeratorToVRMessage& resp,
                                     int claim_id, const std::string& username) {
        debate::Claim c = getClaimFromResponse(resp, claim_id);
        if (c.id() == 0) return debate::ClaimStatus::UNDETERMINED;
        auto it = c.user_statuses().find(username);
        if (it != c.user_statuses().end()) return it->second;
        return debate::ClaimStatus::UNDETERMINED;
    }

    // Count links of a given type in a response collection
    int countLinksByType(const moderator_to_vr::ModeratorToVRMessage& resp, debate::LinkType type) {
        int count = 0;
        for (const auto& entry : resp.collection().links_by_id()) {
            if (entry.second.link_type() == type) count++;
        }
        return count;
    }

    // Get all responses (one per user in user_ids_)
    std::map<std::string, moderator_to_vr::ModeratorToVRMessage> getAllResponses() {
        std::map<std::string, moderator_to_vr::ModeratorToVRMessage> responses;
        for (const auto& kv : user_ids_) {
            debate_event::DebateEvent event;
            event.mutable_user()->set_user_id(kv.second);
            event.mutable_user()->set_username(kv.first);
            event.mutable_user()->set_is_logged_in(true);
            event.set_type(debate_event::NONE);
            responses[kv.first] = moderator_->handleRequest(event);
        }
        return responses;
    }

    // Execute a single TestScenario and check all expectations
    void executeScenario(const TestScenario& scenario) {
        // Create initial users A, B, C
        getUserId("A");
        getUserId("B");
        getUserId("C");

        std::string debate_topic = scenario.debate_topic();

        // Execute all actions
        for (const auto& action : scenario.actions()) {
            int user_id = getUserId(action.username());
            debate_event::DebateEvent event = forgeEvent(action);
            moderator_->handleRequest(event);
        }

        // Get responses from all users
        auto responses = getAllResponses();

        // Check expectations
        for (const auto& exp : scenario.expectations()) {
            const std::string& username = exp.username();
            auto resp_it = responses.find(username);
            ASSERT_TRUE(resp_it != responses.end())
                << "Expectation references unknown user: " << username;
            const auto& resp = resp_it->second;

            if (exp.check_type() == "CLAIM_EXISTS") {
                debate::Claim c = getClaimFromResponse(resp, exp.claim_id());
                EXPECT_GT(c.id(), 0)
                    << "[" << scenario.name() << "] CLAIM_EXISTS claim_id=" << exp.claim_id()
                    << " for user " << username;
            }
            else if (exp.check_type() == "CLAIM_SENTENCE") {
                debate::Claim c = getClaimFromResponse(resp, exp.claim_id());
                EXPECT_EQ(c.sentence(), exp.expected_claim_sentence())
                    << "[" << scenario.name() << "] CLAIM_SENTENCE claim_id=" << exp.claim_id()
                    << " for user " << username;
            }
            else if (exp.check_type() == "USER_VIEW") {
                debate::ClaimStatus actual = getUserView(resp, exp.claim_id(), username);
                debate::ClaimStatus expected = parseClaimStatus(exp.expected_status());
                EXPECT_EQ(actual, expected)
                    << "[" << scenario.name() << "] USER_VIEW claim_id=" << exp.claim_id()
                    << " user=" << username
                    << " expected=" << exp.expected_status();
            }
            else if (exp.check_type() == "LINK_COUNT") {
                debate::LinkType lt = parseLinkType(exp.link_type());
                int actual = countLinksByType(resp, lt);
                EXPECT_EQ(actual, exp.expected_count())
                    << "[" << scenario.name() << "] LINK_COUNT type=" << exp.link_type()
                    << " for user " << username;
            }
            else if (exp.check_type() == "LINK_EXISTS") {
                bool found = false;
                for (const auto& entry : resp.collection().links_by_id()) {
                    const debate::Link& link = entry.second;
                    if (link.link_type() == parseLinkType(exp.link_type()) &&
                        link.connect_from() == exp.from_claim_id() &&
                        link.connect_to() == exp.to_claim_id()) {
                        found = true;
                        break;
                    }
                }
                EXPECT_TRUE(found)
                    << "[" << scenario.name() << "] LINK_EXISTS "
                    << exp.from_claim_id() << " -> " << exp.to_claim_id()
                    << " type=" << exp.link_type() << " for user " << username;
            }
            else if (exp.check_type() == "ENGAGEMENT_STATE") {
                user_engagement::EngagementAction actual = resp.user().engagement().current_action();
                user_engagement::EngagementAction expected = parseUserAction(exp.expected_action());
                EXPECT_EQ(actual, expected)
                    << "[" << scenario.name() << "] ENGAGEMENT_STATE for user " << username
                    << " expected=" << exp.expected_action();
            }
            else if (exp.check_type() == "COLLECTION_SIZE") {
                int actual = resp.collection().claims_by_id_size();
                EXPECT_EQ(actual, exp.expected_count())
                    << "[" << scenario.name() << "] COLLECTION_SIZE for user " << username;
            }
        }
    }

    // Load a TestScenario from a binary protobuf file
    TestScenario loadScenario(const std::string& path) {
        TestScenario scenario;
        std::ifstream file(path, std::ios::binary);
        EXPECT_TRUE(file.is_open()) << "Failed to open scenario file: " << path;
        if (file.is_open()) {
            scenario.ParseFromIstream(&file);
        }
        return scenario;
    }

    std::string db_path_;
    DebateModerator* moderator_ = nullptr;
    std::map<std::string, int> user_ids_;
};

// ============================================================
// Test: ChallengeClaim scenario
// ============================================================
TEST_F(ScenarioRunner, ChallengeClaim) {
    TestScenario scenario;
    scenario.set_name("ChallengeClaim");
    scenario.set_debate_topic("Is AI beneficial?");

    // A creates debate
    auto* a1 = scenario.add_actions();
    a1->set_username("A");
    a1->set_event_type("CREATE_DEBATE");
    a1->set_debate_topic("Is AI beneficial?");

    // A enters + joins
    auto* a2 = scenario.add_actions();
    a2->set_username("A");
    a2->set_event_type("ENTER_DEBATE");
    a2->set_debate_id(1);
    auto* a3 = scenario.add_actions();
    a3->set_username("A");
    a3->set_event_type("JOIN_DEBATE");
    a3->set_debate_id(1);

    // B enters + joins
    auto* b1 = scenario.add_actions();
    b1->set_username("B");
    b1->set_event_type("ENTER_DEBATE");
    b1->set_debate_id(1);
    auto* b2 = scenario.add_actions();
    b2->set_username("B");
    b2->set_event_type("JOIN_DEBATE");
    b2->set_debate_id(1);

    // A adds child claim
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

    // B navigates to child and challenges it
    auto* b3 = scenario.add_actions();
    b3->set_username("B");
    b3->set_event_type("GO_TO_CLAIM");
    b3->set_claim_id(2);
    auto* b4 = scenario.add_actions();
    b4->set_username("B");
    b4->set_event_type("SUBMIT_CHALLENGE_CLAIM");
    b4->set_claim_text("Healthcare AI has bias issues");

    // --- Expectations ---

    // Child claim exists for B
    auto* e1 = scenario.add_expectations();
    e1->set_check_type("CLAIM_EXISTS");
    e1->set_username("B");
    e1->set_claim_id(2);

    // Child sentence correct
    auto* e2 = scenario.add_expectations();
    e2->set_check_type("CLAIM_SENTENCE");
    e2->set_username("A");
    e2->set_claim_id(2);
    e2->set_expected_claim_sentence("AI improves healthcare");

    // A sees child as TRUE_CLAIM
    auto* e3 = scenario.add_expectations();
    e3->set_check_type("USER_VIEW");
    e3->set_username("A");
    e3->set_claim_id(2);
    e3->set_expected_status("TRUE_CLAIM");

    // B sees child as FALSE_CLAIM
    auto* e4 = scenario.add_expectations();
    e4->set_check_type("USER_VIEW");
    e4->set_username("B");
    e4->set_claim_id(2);
    e4->set_expected_status("FALSE_CLAIM");

    // 1 PARENT_CHILD link, 1 CHALLENGE link in B's collection
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

    // Both users in debating state
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
