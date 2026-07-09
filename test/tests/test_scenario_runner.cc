// test_scenario_runner.cc — Data-driven test runner for TestScenario protobufs
//
// ARCHITECTURE OVERVIEW
// =====================
// Each TestScenario is a list of TestSteps.
// Each TestStep has an optional action and optional expectations.
// Steps execute sequentially — action runs, then expectations are checked.
//
// This means you can verify state at any intermediate point, not just at the end.
//
// ACTION TYPES
// ===========
// "CREATE_USER" — Special runner-level action. Calls createUserIfNotExist().
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
#include "utils/Log.h"
#include "debateModerator/DebateModerator.h"
#include "debateModerator/buildResponse/debatePageResponse/BuildCollection.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/json/json.h"
#include "virtualRenderer/LayoutGenerator/pages/debatePage/StepView/StepView.h"
#include "virtualRenderer/LayoutGenerator/pages/debatePage/FullDebateView/FullDebatePageInfoParser.h"
#include "database/virtualrenderer/VRUserDatabase.h"

using debate_test::TestScenario;
using debate_test::TestStep;
using debate_test::TestAction;
using debate_test::TestExpectation;

// Forward declaration — defined in test_step_view_layout.cc
void downloadJson(int debate_id, const std::string& viewer_username = "A",
                  const std::string& output_path = "",
                  const std::string& description = "");


// ===========================================================================
// SECTION 1: TEST FIXTURE
// ===========================================================================

class ScenarioRunner : public ::testing::Test {
protected:

    // -----------------------------------------------------------------------
    // SECTION 1a: Lifecycle (SetUp / TearDown)
    // -----------------------------------------------------------------------

    void SetUp() override {
        db_path_ = "test_scenario.sqlite3";
        std::remove(db_path_.c_str());
        std::remove((db_path_ + "-wal").c_str());
        std::remove((db_path_ + "-shm").c_str());
        _putenv_s("DB_PATH", db_path_.c_str());
        Log::init(LogLevel::Test);
        moderator_ = new DebateModerator();
        db_ = new Database(db_path_);
        dbWrapper_ = new DatabaseWrapper(*db_);
        debateWrapper_ = new DebateWrapper(*dbWrapper_);
    }

    void TearDown() override {
        delete debateWrapper_;
        delete dbWrapper_;
        delete db_;
        delete moderator_;
        _putenv_s("DB_PATH", "");
    }


    // -----------------------------------------------------------------------
    // SECTION 1b: User Management
    // -----------------------------------------------------------------------

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

    std::string claimStatusToString(debate::ClaimStatus s) {
        if (s == debate::ClaimStatus::TRUE_CLAIM) return "TRUE_CLAIM";
        if (s == debate::ClaimStatus::FALSE_CLAIM) return "FALSE_CLAIM";
        return "UNDETERMINED";
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

    // Scenario files never recorded which specific CHALLENGE link a concede
    // targets (autoincrement link ids aren't known when authoring a .pbtxt).
    // Production ChallengeHandler::ConcedeChallenge now requires an explicit,
    // valid challenge_link_id and errors otherwise — so the harness resolves
    // it here instead, standing in for what a real frontend's UI state would
    // supply: the CHALLENGE link whose target claim was created by the
    // conceding user (highest link id wins if more than one matches).
    int resolveChallengeLinkIdForUser(int debate_id, int conceding_user_id) {
        int bestFromClaimId = -1;
        int bestLinkId = -1;
        auto allLinks = debateWrapper_->getLinksForDebate(debate_id);
        for (const auto& linkTuple : allLinks) {
            int linkId = std::get<0>(linkTuple);
            int fromClaimId = std::get<1>(linkTuple);
            int toClaimId = std::get<2>(linkTuple);
            int linkType = std::get<5>(linkTuple);
            if (linkType != static_cast<int>(debate::LinkType::CHALLENGE)) continue;
            debate::Claim targetClaim = debateWrapper_->getClaimById(toClaimId);
            if (targetClaim.id() != 0 && targetClaim.creator_id() == conceding_user_id) {
                if (fromClaimId > bestFromClaimId) {
                    bestFromClaimId = fromClaimId;
                    bestLinkId = linkId;
                }
            }
        }
        return bestLinkId;
    }

    debate_event::DebateEvent forgeEvent(const TestAction& action, int debate_id) {
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
                event.mutable_submit_challenge_claim()->set_challenge_description(
                    action.challenge_description().empty() ? "challenge claim" : action.challenge_description());
                break;
            case debate_event::CONCEDE_CHALLENGE:
                event.mutable_concede_challenge()->set_challenge_link_id(
                    resolveChallengeLinkIdForUser(debate_id, user_id));
                break;
            default:
                break;
        }
        return event;
    }


    // -----------------------------------------------------------------------
    // SECTION 4: COLLECTION BUILDER
    // -----------------------------------------------------------------------

    debate::Collection buildCollectionForAllUsers(int debate_id) {
        std::vector<int> user_ids;
        for (const auto& kv : user_ids_) {
            user_ids.push_back(kv.second);
        }
        return BuildCollection::BuildForDebateAndUsers(debate_id, user_ids, *debateWrapper_);
    }


    // -----------------------------------------------------------------------
    // SECTION 5: COLLECTION QUERY HELPERS
    // -----------------------------------------------------------------------

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
            if (entry.second.link().link_type() == type) count++;
        }
        return count;
    }

    bool linkExists(const debate::Collection& collection, debate::LinkType type,
                    int from_claim_id, int to_claim_id) {
        for (const auto& entry : collection.links_by_id()) {
            const debate::Relationship::Link& link = entry.second.link();
            if (link.link_type() == type &&
                link.connect_from() == from_claim_id &&
                link.connect_to() == to_claim_id) {
                return true;
            }
        }
        return false;
    }

    int findNthChallengeClaimId(const debate::Collection& collection, int target_claim_id,
                                 debate::LinkType link_type, int n, const std::vector<int>& skip_ids) {
        int found = 0;
        for (const auto& entry : collection.links_by_id()) {
            const debate::Relationship::Link& link = entry.second.link();
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
    // SECTION 6b: EXPECTATION CHECKER (saves page JSON on failure)
    // -----------------------------------------------------------------------
    // Checks a single expectation against the current state.
    // On failure, calls downloadJson to save the rendered page as JSON.

    void checkExpectation(const TestExpectation& exp,
                          const std::map<std::string, moderator_to_vr::ModeratorToVRMessage>& last_responses,
                          const debate::Collection& collection,
                          const std::string& scenario_name,
                          int step_index,
                          int debate_id) {

        auto savePageJson = [&]() {
            if (debate_id > 0) {
                std::string filename = "FAIL_" + scenario_name + "_step" + std::to_string(step_index) + ".json";
                downloadJson(debate_id, "A", filename, "FAILED expectation at step " + std::to_string(step_index));
            }
        };

        if (exp.check_type() == "ENGAGEMENT_STATE") {
            Log::test("  CHECK ENGAGEMENT_STATE " + exp.username() +
                      " expected=" + exp.expected_action());
            auto resp_it = last_responses.find(exp.username());
            if (resp_it == last_responses.end()) {
                savePageJson();
            }
            ASSERT_TRUE(resp_it != last_responses.end())
                << "[" << scenario_name << "] ENGAGEMENT_STATE unknown user: " << exp.username();
            auto actual = resp_it->second.user().engagement().current_action();
            auto expected = parseEngagementAction(exp.expected_action());
            if (actual != expected) {
                savePageJson();
                Log::error("  FAIL actual=" + actual);
            } else {
                Log::test("  PASS");
            }
            EXPECT_EQ(actual, expected)
                << "[" << scenario_name << "] ENGAGEMENT_STATE " << exp.username();
            return;
        }

        if (exp.check_type() == "CLAIM_EXISTS") {
            Log::test("  CHECK CLAIM_EXISTS " + std::to_string(exp.claim_id()));
            if (getClaim(collection, exp.claim_id()).id() == 0) {
                savePageJson();
                Log::error("  FAIL (not found)");
            } else {
                Log::test("  PASS");
            }
            EXPECT_GT(getClaim(collection, exp.claim_id()).id(), 0)
                << "[" << scenario_name << "] CLAIM_EXISTS " << exp.claim_id();
        }
        else if (exp.check_type() == "CLAIM_SENTENCE") {
            Log::test("  CHECK CLAIM_SENTENCE " + std::to_string(exp.claim_id()) +
                      " expected=" + exp.expected_claim_sentence());
            auto actual = getClaim(collection, exp.claim_id()).sentence();
            if (actual != exp.expected_claim_sentence()) {
                savePageJson();
                Log::error("  FAIL actual=" + actual);
            } else {
                Log::test("  PASS");
            }
            EXPECT_EQ(actual, exp.expected_claim_sentence())
                << "[" << scenario_name << "] CLAIM_SENTENCE " << exp.claim_id();
        }
        else if (exp.check_type() == "USER_VIEW") {
            Log::test("  CHECK USER_VIEW " + std::to_string(exp.claim_id()) +
                      " " + exp.username() + " expected=" + exp.expected_status());
            auto actual = getUserView(collection, exp.claim_id(), exp.username());
            auto expected = parseClaimStatus(exp.expected_status());
            if (actual != expected) {
                savePageJson();
                Log::error("  FAIL actual=" + claimStatusToString(actual));
            } else {
                Log::test("  PASS");
            }
            EXPECT_EQ(actual, expected)
                << "[" << scenario_name << "] USER_VIEW " << exp.claim_id() << " " << exp.username();
        }
        else if (exp.check_type() == "LINK_COUNT") {
            Log::test("  CHECK LINK_COUNT " + exp.link_type() +
                      " expected=" + std::to_string(exp.expected_count()));
            auto lt = parseLinkType(exp.link_type());
            auto actual = countLinks(collection, lt);
            if (actual != exp.expected_count()) {
                savePageJson();
                Log::error("  FAIL actual=" + std::to_string(actual));
            } else {
                Log::test("  PASS");
            }
            EXPECT_EQ(actual, exp.expected_count())
                << "[" << scenario_name << "] LINK_COUNT " << exp.link_type();
        }
        else if (exp.check_type() == "LINK_EXISTS") {
            Log::test("  CHECK LINK_EXISTS " + exp.link_type() + " " +
                      std::to_string(exp.from_claim_id()) + " -> " +
                      std::to_string(exp.to_claim_id()));
            auto lt = parseLinkType(exp.link_type());
            if (!linkExists(collection, lt, exp.from_claim_id(), exp.to_claim_id())) {
                savePageJson();
                Log::error("  FAIL (link not found)");
            } else {
                Log::test("  PASS");
            }
            EXPECT_TRUE(linkExists(collection, lt, exp.from_claim_id(), exp.to_claim_id()))
                << "[" << scenario_name << "] LINK_EXISTS "
                << exp.from_claim_id() << " -> " << exp.to_claim_id();
        }
        else if (exp.check_type() == "COLLECTION_SIZE") {
            Log::test("  CHECK COLLECTION_SIZE expected=" + std::to_string(exp.expected_count()));
            if (collection.claims_by_id_size() != exp.expected_count()) {
                savePageJson();
                Log::error("  FAIL actual=" + std::to_string(collection.claims_by_id_size()));
            } else {
                Log::test("  PASS");
            }
            EXPECT_EQ(collection.claims_by_id_size(), exp.expected_count())
                << "[" << scenario_name << "] COLLECTION_SIZE";
        }
    }


    // -----------------------------------------------------------------------
    // SECTION 7: SCENARIO EXECUTOR
    // -----------------------------------------------------------------------
    // Iterates through steps in order.
    // For each step: execute action (if present), then check expectations (if present).
    //
    // dump_json_per_step: if true, saves a JSON snapshot after every action step
    //   (filename: STEP_<scenario>_step<N>.json) so you can inspect the page state
    //   at each intermediate point.

    void executeScenario(const TestScenario& scenario, bool dump_json_per_step = false) {
        // Track per-user last response for ENGAGEMENT_STATE checks
        std::map<std::string, moderator_to_vr::ModeratorToVRMessage> last_responses;
        // Track the debate_id from actions for collection building
        int debate_id = 0;
        int step_index = 0;

        for (const auto& step : scenario.steps()) {
            // --- Execute action (if present) ---
            if (step.has_action()) {
                const TestAction& action = step.action();
                Log::test("[STEP " + std::to_string(step_index) + "] ACTION: " +
                         action.username() + " -> " + action.event_type());

                if (action.event_type() == "CREATE_USER") {
                    getUserId(action.username());
                } else {
                    int user_id = getUserId(action.username());
                    debate_event::DebateEvent event = forgeEvent(action, debate_id);
                    auto resp = moderator_->handleRequest(event);
                    last_responses[action.username()] = resp;

                    // Track debate_id from ENTER_DEBATE or JOIN_DEBATE
                    if (action.event_type() == "ENTER_DEBATE" || action.event_type() == "JOIN_DEBATE") {
                        if (action.debate_id() > 0) debate_id = action.debate_id();
                    }
                }

                // Dump JSON after every action step (if enabled)
                if (dump_json_per_step && debate_id > 0) {
                    // Build a description from the action
                    const TestAction& action = step.action();
                    std::string desc = "Step " + std::to_string(step_index) + ": " +
                        action.username() + " " + action.event_type();
                    if (!action.debate_topic().empty()) {
                        desc += " [topic: " + action.debate_topic() + "]";
                    }
                    if (!action.claim_text().empty()) {
                        desc += " [text: " + action.claim_text() + "]";
                    }
                    if (action.claim_id() > 0) {
                        desc += " [claim_id: " + std::to_string(action.claim_id()) + "]";
                    }
                    if (action.debate_id() > 0) {
                        desc += " [debate_id: " + std::to_string(action.debate_id()) + "]";
                    }
                    // Create per-scenario output directory
                    std::string outDir = "json_output/" + scenario.name();
                    std::filesystem::create_directories(outDir);
                    std::string filename = outDir + "/STEP_" + scenario.name() + "_step" + std::to_string(step_index) + ".json";
                    // Use first user as viewer (fallback to "A")
                    std::string viewer = "A";
                    if (!user_ids_.empty()) viewer = user_ids_.begin()->first;
                    downloadJson(debate_id, viewer, filename, desc);
                }
            }

            // --- Check expectations (if any) ---
            if (step.expectations_size() > 0) {
                // Build collection for expectation checks
                debate::Collection collection;
                if (debate_id > 0) {
                    collection = buildCollectionForAllUsers(debate_id);
                }

                for (const auto& exp : step.expectations()) {
                    checkExpectation(exp, last_responses, collection, scenario.name(), step_index, debate_id);
                }
            }
            step_index++;
        }

        // Save final state as JSON after scenario completes (pass or fail)
        if (debate_id > 0) {
            std::string filename = "FINAL_" + scenario.name() + ".json";
            // Use the first user in the scenario as viewer (fallback to "A")
            std::string viewer = "A";
            if (!user_ids_.empty()) {
                viewer = user_ids_.begin()->first;
            }
            downloadJson(debate_id, viewer, filename);
        }
    }


    // -----------------------------------------------------------------------
    // SECTION 8: FIXTURE MEMBER VARIABLES
    // -----------------------------------------------------------------------

    std::string db_path_;
    DebateModerator* moderator_ = nullptr;
    // Standalone DB handle wired to the same DB_PATH as moderator_, used only
    // to reach a DebateWrapper for read-only collection building in tests
    // (DebateModerator no longer exposes its internal DebateWrapper).
    Database* db_ = nullptr;
    DatabaseWrapper* dbWrapper_ = nullptr;
    DebateWrapper* debateWrapper_ = nullptr;
    std::map<std::string, int> user_ids_;


    // -----------------------------------------------------------------------
    // SECTION 8a: LOAD SCENARIO FROM PBTXT FILE
    // -----------------------------------------------------------------------

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
// SECTION 9: TEST CASES
// ===========================================================================

// ---------------------------------------------------------------------------
// ComprehensiveTest — Full lifecycle scenario (create→add children→challenge→counter→concede)
// ---------------------------------------------------------------------------
TEST_F(ScenarioRunner, ComprehensiveTest) {
    TestScenario s = LoadScenarioFromFile("scenarios/ComprehensiveTest.pbtxt");
    ASSERT_GT(s.steps_size(), 0) << "Failed to load ComprehensiveTest.pbtxt";
    // Dump JSON after every action step — files: STEP_ComprehensiveTest_step0.json, step1.json, ...
    executeScenario(s, /*dump_json_per_step=*/true);
}

TEST_F(ScenarioRunner, TestA) {
    TestScenario s = LoadScenarioFromFile("scenarios/TestA.pbtxt");
    ASSERT_GT(s.steps_size(), 0) << "Failed to load TestA.pbtxt";
    executeScenario(s, /*dump_json_per_step=*/true);
}

TEST_F(ScenarioRunner, SeaLevelDebate) {
    TestScenario s = LoadScenarioFromFile("scenarios/SeaLevelDebate.pbtxt");
    ASSERT_GT(s.steps_size(), 0) << "Failed to load SeaLevelDebate.pbtxt";
    executeScenario(s, /*dump_json_per_step=*/true);
}

TEST_F(ScenarioRunner, GlobalWarmingHoaxDebate) {
    TestScenario s = LoadScenarioFromFile("scenarios/GlobalWarmingHoaxDebate.pbtxt");
    ASSERT_GT(s.steps_size(), 0) << "Failed to load GlobalWarmingHoaxDebate.pbtxt";
    executeScenario(s, /*dump_json_per_step=*/true);
}

TEST_F(ScenarioRunner, ClimateChangeDebate) {
    TestScenario s = LoadScenarioFromFile("scenarios/ClimateChangeDebate.pbtxt");
    ASSERT_GT(s.steps_size(), 0) << "Failed to load ClimateChangeDebate.pbtxt";
    executeScenario(s, /*dump_json_per_step=*/true);
}

TEST_F(ScenarioRunner, ClimateChangeDemo) {
    TestScenario s = LoadScenarioFromFile("scenarios/ClimateChangeDemo.pbtxt");
    ASSERT_GT(s.steps_size(), 0) << "Failed to load ClimateChangeDemo.pbtxt";
    executeScenario(s, /*dump_json_per_step=*/true);
}

TEST_F(ScenarioRunner, ConcedePropagationTest) {
    TestScenario s = LoadScenarioFromFile("scenarios/ConcedePropagationTest.pbtxt");
    ASSERT_GT(s.steps_size(), 0) << "Failed to load ConcedePropagationTest.pbtxt";
    executeScenario(s, /*dump_json_per_step=*/true);
}

TEST_F(ScenarioRunner, MegaTest) {
    TestScenario s = LoadScenarioFromFile("scenarios/MegaTest.pbtxt");
    ASSERT_GT(s.steps_size(), 0) << "Failed to load MegaTest.pbtxt";
    executeScenario(s, /*dump_json_per_step=*/true);
}
