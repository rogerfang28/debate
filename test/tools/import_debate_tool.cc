// import_debate_tool.cc — Standalone CLI: import a .pbtxt TestScenario into a live debate DB
//
// Loads a debate_test::TestScenario from a .pbtxt file and replays every action step
// through the same DebateModerator::handleRequest() pipeline the scenario test runner
// uses (test/tests/test_scenario_runner.cc) — but against a real, persistent database
// instead of a throwaway per-test sqlite file. TestExpectation blocks in the scenario
// are ignored; this tool only cares about executing actions.
//
// Usage:
//   import_debate_tool <scenario.pbtxt> [db_path]
//
// db_path is optional. If omitted, the DB_PATH environment variable is used if set,
// otherwise DebateModerator falls back to its default database location
// (see utils::getDatabasePath()).
//
// Exits 0 on success, non-zero if the file can't be loaded/parsed or any step fails.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

#include "debate.pb.h"
#include "debate_event.pb.h"
#include "test_scenario.pb.h"
#include "google/protobuf/text_format.h"
#include "debateModerator/DebateModerator.h"
#include "utils/DebateWrapper.h"
#include "utils/Log.h"
#include "utils/pathUtils.h"

using debate_test::TestScenario;
using debate_test::TestStep;
using debate_test::TestAction;

namespace {

TestScenario LoadScenarioFromFile(const std::string& path, bool& ok) {
    TestScenario scenario;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[import_debate_tool] FAILED to open scenario file: " << path << std::endl;
        ok = false;
        return scenario;
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    ok = google::protobuf::TextFormat::ParseFromString(content, &scenario);
    if (!ok) {
        std::cerr << "[import_debate_tool] FAILED to parse scenario file as TestScenario pbtxt: " << path << std::endl;
    }
    return scenario;
}

// Mirrors ScenarioRunner::resolveChallengeLinkIdForUser in test_scenario_runner.cc:
// scenario files don't record which specific CHALLENGE link a concede targets, so we
// resolve it here — the CHALLENGE link whose target claim was created by the
// conceding user (highest link id wins if more than one matches).
int resolveChallengeLinkIdForUser(DebateWrapper& debateWrapper, int debate_id, int conceding_user_id) {
    int bestFromClaimId = -1;
    int bestLinkId = -1;
    auto allLinks = debateWrapper.getLinksForDebate(debate_id);
    for (const auto& linkTuple : allLinks) {
        int linkId = std::get<0>(linkTuple);
        int fromClaimId = std::get<1>(linkTuple);
        int toClaimId = std::get<2>(linkTuple);
        int linkType = std::get<5>(linkTuple);
        if (linkType != static_cast<int>(debate::LinkType::CHALLENGE)) continue;
        debate::Claim targetClaim = debateWrapper.getClaimById(toClaimId);
        if (targetClaim.id() != 0 && targetClaim.creator_id() == conceding_user_id) {
            if (fromClaimId > bestFromClaimId) {
                bestFromClaimId = fromClaimId;
                bestLinkId = linkId;
            }
        }
    }
    return bestLinkId;
}

// A small importer that walks a TestScenario and executes each action against
// a live DebateModerator, mirroring ScenarioRunner::forgeEvent()/executeScenario()
// from the gtest-based scenario runner — minus expectation checking.
class ScenarioImporter {
public:
    explicit ScenarioImporter(DebateModerator& moderator) : moderator_(moderator) {}

    // Returns true if every step executed successfully.
    bool Run(const TestScenario& scenario) {
        int step_index = 0;
        for (const auto& step : scenario.steps()) {
            if (step.has_action()) {
                if (!RunAction(step.action(), step_index)) {
                    std::cerr << "[import_debate_tool] FAILED at step " << step_index << std::endl;
                    return false;
                }
            }
            step_index++;
        }
        std::cout << "[import_debate_tool] Imported " << step_index << " step(s) successfully."
                   << " debate_id=" << debate_id_ << std::endl;
        return true;
    }

private:
    int GetUserId(const std::string& username) {
        auto it = user_ids_.find(username);
        if (it != user_ids_.end()) return it->second;
        int id = moderator_.createUserIfNotExist(username);
        user_ids_[username] = id;
        return id;
    }

    bool RunAction(const TestAction& action, int step_index) {
        std::cout << "[STEP " << step_index << "] " << action.username()
                   << " -> " << action.event_type() << std::endl;

        if (action.event_type() == "CREATE_USER") {
            int id = GetUserId(action.username());
            std::cout << "  created/found user '" << action.username() << "' (id=" << id << ")" << std::endl;
            return true;
        }

        debate_event::EventType type;
        if (!debate_event::EventType_Parse(action.event_type(), &type)) {
            std::cerr << "  ERROR: unrecognized event_type '" << action.event_type() << "'" << std::endl;
            return false;
        }

        if (type == debate_event::CONCEDE_CHALLENGE && debate_id_ == 0) {
            std::cerr << "  ERROR: CONCEDE_CHALLENGE requires a debate to already be entered" << std::endl;
            return false;
        }

        int user_id = GetUserId(action.username());

        // Snapshot before, so the claims this step creates can be identified by
        // difference afterwards -- the handlers do not report what they made.
        const bool creates_claims = (type == debate_event::CREATE_DEBATE ||
                                     type == debate_event::ADD_CHILD_CLAIM ||
                                     type == debate_event::SUBMIT_CHALLENGE_CLAIM);
        std::set<int> claims_before;
        if (creates_claims && debate_id_ > 0) {
            claims_before = ClaimIdsInDebate(debate_id_);
        }

        debate_event::DebateEvent event = ForgeEvent(action, type, user_id);

        if (type == debate_event::CONCEDE_CHALLENGE &&
            event.concede_challenge().challenge_link_id() <= 0) {
            std::cerr << "  ERROR: could not resolve a CHALLENGE link for user '"
                       << action.username() << "' to concede" << std::endl;
            return false;
        }

        moderator_.handleRequest(event);

        // A scenario writes `debate_id: 1` because a test database is empty and
        // the first debate really is 1. A LIVE database already has debates, so
        // the debate this scenario just created is not 1 -- and taking the
        // literal id means joining somebody else's debate and grafting the
        // whole argument onto their claims. That is silent: every step still
        // "succeeds".
        //
        // So the created debate is recorded here and the scenario's ids are
        // translated in ForgeEvent. Debates are numbered by creation order in
        // both worlds, which is what makes the mapping sound.
        if (type == debate_event::CREATE_DEBATE) {
            const std::vector<int> ids =
                moderator_.getDebateWrapper().getUserDebateIds(user_id);
            if (!ids.empty()) {
                const int real_id = ids.back();
                scenario_to_real_debate_[static_cast<int>(scenario_to_real_debate_.size()) + 1] =
                    real_id;
                debate_id_ = real_id;
                if (real_id != static_cast<int>(scenario_to_real_debate_.size())) {
                    std::cout << "  (scenario debate "
                              << scenario_to_real_debate_.size() << " -> real debate "
                              << real_id << ")" << std::endl;
                }
            }
        }

        if (action.event_type() == "ENTER_DEBATE" || action.event_type() == "JOIN_DEBATE") {
            if (action.debate_id() > 0) debate_id_ = RealDebateId(action.debate_id());
        }

        // Map the claims this step created onto the ids the scenario assumes.
        //
        // A scenario counts claims from 1 because a test database starts empty.
        // In a live database the same claims get whatever ids are next, so
        // GO_TO_CLAIM would otherwise navigate to a stranger's claim and hang
        // the whole argument off it -- silently, since every step still
        // succeeds. New ids are taken in ascending order, which matches the
        // order a scenario numbers them.
        if (creates_claims && debate_id_ > 0) {
            const std::set<int> claims_after = ClaimIdsInDebate(debate_id_);
            for (const int id : claims_after) {
                if (claims_before.count(id) == 0) {
                    scenario_to_real_claim_[next_scenario_claim_id_++] = id;
                }
            }
        }

        PrintStepDetail(action, type);
        return true;
    }

    debate_event::DebateEvent ForgeEvent(const TestAction& action, debate_event::EventType type, int user_id) {
        debate_event::DebateEvent event;
        event.mutable_user()->set_user_id(user_id);
        event.mutable_user()->set_username(action.username());
        event.mutable_user()->set_is_logged_in(true);
        event.set_type(type);

        switch (type) {
            case debate_event::CREATE_DEBATE:
                event.mutable_create_debate()->set_debate_topic(
                    action.debate_topic().empty() ? "Untitled Debate" : action.debate_topic());
                break;
            case debate_event::ENTER_DEBATE:
                event.mutable_enter_debate()->set_debate_id(RealDebateId(action.debate_id()));
                break;
            case debate_event::JOIN_DEBATE:
                event.mutable_join_debate()->set_debate_id(RealDebateId(action.debate_id()));
                break;
            case debate_event::GO_TO_CLAIM:
                event.mutable_go_to_claim()->set_claim_id(RealClaimId(action.claim_id()));
                break;
            // NOTE: this switch mirrors forgeEvent() in
            // test/tests/test_scenario_runner.cc. The two are separate copies
            // and have already drifted once -- extra_claim_texts and basis
            // landed in the runner and not here, so importing a multi-from
            // scenario silently produced single-claim relations with no basis.
            // It did not error; it just built a quieter, wrong graph. Keep
            // them in step, or merge them.
            case debate_event::ADD_CHILD_CLAIM:
                event.mutable_add_child_claim()->set_claim(action.claim_text());
                event.mutable_add_child_claim()->set_description(action.claim_description());
                // Several texts = ONE relation with several from-claims.
                for (const auto& extra : action.extra_claim_texts()) {
                    event.mutable_add_child_claim()->add_extra_claims(extra);
                }
                if (!action.basis().empty()) {
                    event.mutable_add_child_claim()->set_basis(action.basis());
                }
                break;
            case debate_event::SUBMIT_CHALLENGE_CLAIM:
                event.mutable_submit_challenge_claim()->set_challenge_sentence(action.claim_text());
                event.mutable_submit_challenge_claim()->set_challenge_description(action.challenge_description());
                // Same two fields, same meanings: several sentences are ONE
                // opposing relation, and basis says what kind of opposition.
                for (const auto& extra : action.extra_claim_texts()) {
                    event.mutable_submit_challenge_claim()->add_extra_sentences(extra);
                }
                if (!action.basis().empty()) {
                    event.mutable_submit_challenge_claim()->set_basis(action.basis());
                }
                break;
            case debate_event::CONCEDE_CHALLENGE:
                event.mutable_concede_challenge()->set_challenge_link_id(
                    resolveChallengeLinkIdForUser(moderator_.getDebateWrapper(), debate_id_, user_id));
                break;
            default:
                break;
        }
        return event;
    }

    void PrintStepDetail(const TestAction& action, debate_event::EventType type) {
        switch (type) {
            case debate_event::CREATE_DEBATE:
                std::cout << "  created debate: \"" << action.debate_topic() << "\"" << std::endl;
                break;
            case debate_event::ENTER_DEBATE:
                std::cout << "  entered debate " << action.debate_id() << std::endl;
                break;
            case debate_event::JOIN_DEBATE:
                std::cout << "  joined debate " << action.debate_id() << std::endl;
                break;
            case debate_event::GO_TO_CLAIM:
                std::cout << "  moved to claim " << action.claim_id() << std::endl;
                break;
            // The extra members and the basis are printed because they are the
            // difference between one relation and several, and between a
            // definitional challenge and an evidential one. A run that dropped
            // them would otherwise look identical to one that kept them.
            case debate_event::ADD_CHILD_CLAIM:
                std::cout << "  added child claim: \"" << action.claim_text() << "\"";
                for (const auto& extra : action.extra_claim_texts()) {
                    std::cout << "\n                 + \"" << extra << "\"";
                }
                if (action.extra_claim_texts_size() > 0) {
                    std::cout << "\n                 (one relation, "
                              << (action.extra_claim_texts_size() + 1) << " from-claims)";
                }
                if (!action.basis().empty()) {
                    std::cout << "  [" << action.basis() << "]";
                }
                std::cout << std::endl;
                break;
            case debate_event::SUBMIT_CHALLENGE_CLAIM:
                std::cout << "  submitted challenge: \"" << action.claim_text() << "\"";
                for (const auto& extra : action.extra_claim_texts()) {
                    std::cout << "\n                 + \"" << extra << "\"";
                }
                if (action.extra_claim_texts_size() > 0) {
                    std::cout << "\n                 (one relation, "
                              << (action.extra_claim_texts_size() + 1) << " from-claims)";
                }
                if (!action.basis().empty()) {
                    std::cout << "  [" << action.basis() << "]";
                }
                std::cout << std::endl;
                break;
            case debate_event::CONCEDE_CHALLENGE:
                std::cout << "  conceded challenge" << std::endl;
                break;
            default:
                break;
        }
    }

    DebateModerator& moderator_;
    std::map<std::string, int> user_ids_;
    // Translates a scenario's debate id into the one actually created. Unknown
    // ids pass through unchanged -- a scenario referring to a debate it never
    // created is asking for a pre-existing one, and guessing would be worse
    // than honouring it.
    std::set<int> ClaimIdsInDebate(int debate_id) {
        std::set<int> ids;
        for (const auto& blob : moderator_.getDebateWrapper().getStatementsForDebate(debate_id)) {
            debate::Claim claim;
            if (claim.ParseFromArray(blob.data(), static_cast<int>(blob.size()))) {
                ids.insert(claim.id());
            }
        }
        return ids;
    }

    // Translates a scenario's claim id into the one actually created. Unknown
    // ids pass through, so a scenario pointing at a pre-existing claim is
    // honoured rather than guessed at.
    int RealClaimId(int scenario_claim_id) const {
        auto it = scenario_to_real_claim_.find(scenario_claim_id);
        return it == scenario_to_real_claim_.end() ? scenario_claim_id : it->second;
    }

    int RealDebateId(int scenario_debate_id) const {
        auto it = scenario_to_real_debate_.find(scenario_debate_id);
        return it == scenario_to_real_debate_.end() ? scenario_debate_id : it->second;
    }

    int debate_id_ = 0;
    std::map<int, int> scenario_to_real_debate_;
    std::map<int, int> scenario_to_real_claim_;
    int next_scenario_claim_id_ = 1;
};

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: import_debate_tool <scenario.pbtxt> [db_path] [users_db_path]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "  db_path        debates database. Defaults to DB_PATH, then the built-in path." << std::endl;
        std::cerr << "  users_db_path  users database. Defaults to db_path." << std::endl;
        std::cerr << std::endl;
        std::cerr << "  Production splits these (backend/debates.sqlite3 + backend/users.sqlite3)." << std::endl;
        std::cerr << "  Importing into a live database WITHOUT users_db_path creates the scenario's" << std::endl;
        std::cerr << "  users in the debates file, where the server never looks -- so claims end up" << std::endl;
        std::cerr << "  attributed to whoever happens to hold that id in the real users file." << std::endl;
        return 1;
    }

    std::string scenario_path = argv[1];
    if (argc >= 3) {
        std::string db_path = argv[2];
        _putenv_s("DB_PATH", db_path.c_str());
    }
    // Users default to the debates file, which is what the test harness does
    // and what this tool did originally. Pass it explicitly when importing into
    // a live database, or the scenario's users are created somewhere the server
    // does not read.
    const std::string users_db_path = (argc >= 4) ? argv[3] : utils::getDatabasePath();

    Log::init(LogLevel::Warn);

    bool loaded_ok = false;
    TestScenario scenario = LoadScenarioFromFile(scenario_path, loaded_ok);
    if (!loaded_ok) {
        return 1;
    }
    if (scenario.steps_size() == 0) {
        std::cerr << "[import_debate_tool] Scenario '" << scenario.name() << "' has no steps." << std::endl;
        return 1;
    }

    std::cout << "[import_debate_tool] Importing scenario '" << scenario.name() << "' ("
               << scenario.steps_size() << " step(s))" << std::endl;
    std::cout << "                     debates: " << utils::getDatabasePath() << std::endl;
    std::cout << "                     users:   " << users_db_path << std::endl;

    // DebateModerator takes the USERS database and opens the debates one itself
    // from DB_PATH, exactly as the server does. Keeping them separate here is
    // what lets an import land in the same place the running server reads.
    Database usersDb(users_db_path);
    DebateModerator moderator(usersDb);
    ScenarioImporter importer(moderator);
    bool success = importer.Run(scenario);

    return success ? 0 : 1;
}
