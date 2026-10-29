#include "ScenarioImporter.h"

#include "Log.h"
#include "DebateWrapper.h"
#include "../debateModerator/DebateModerator.h"
#include "debate.pb.h"
#include "debate_event.pb.h"
#include "google/protobuf/text_format.h"

#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>

namespace {

// The moved code was written when the tool had this at file scope. Keeping it
// inside the anonymous namespace preserves the code verbatim without leaking
// the using-directive into anything that includes this.
using namespace debate_test;

// Models wrap output in a fence however firmly they are told not to.
std::string stripCodeFence(const std::string& text) {
    const size_t start = text.find("```");
    if (start == std::string::npos) {
        return text;
    }
    size_t contentStart = text.find('\n', start);
    if (contentStart == std::string::npos) {
        return text;
    }
    contentStart += 1;
    const size_t end = text.find("```", contentStart);
    if (end == std::string::npos) {
        return text.substr(contentStart);
    }
    return text.substr(contentStart, end - contentStart);
}


TestScenario LoadScenarioFromFile(const std::string& path, bool& ok) {
    TestScenario scenario;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[scenario] FAILED to open scenario file: " << path << std::endl;
        ok = false;
        return scenario;
    }
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    ok = google::protobuf::TextFormat::ParseFromString(content, &scenario);
    if (!ok) {
        std::cerr << "[scenario] FAILED to parse scenario file as TestScenario pbtxt: " << path << std::endl;
    }
    return scenario;
}

// The challenge relations `user_id` has conceded, from the move log: each
// CONCEDE carries the relation it gave up as `challenge_relation_id`.
std::set<int> ConcededRelationsByUser(DebateWrapper& debateWrapper, int debate_id, int user_id) {
    std::set<int> out;
    for (const debate::Move& m : debateWrapper.getMovesForDebate(debate_id)) {
        if (m.type() != debate::CONCEDE || m.actor_id() != user_id) continue;
        const std::string needle = "\"challenge_relation_id\":";
        const size_t at = m.payload().find(needle);
        if (at == std::string::npos) continue;
        int id = 0;
        for (size_t i = at + needle.size(); i < m.payload().size(); ++i) {
            const char ch = m.payload()[i];
            if (ch < '0' || ch > '9') break;
            id = id * 10 + (ch - '0');
        }
        if (id > 0) out.insert(id);
    }
    return out;
}

// Mirrors ScenarioRunner::resolveChallengeLinkIdForUser in test_scenario_runner.cc:
// scenario files don't record which specific CHALLENGE link a concede targets, so we
// resolve it here — the CHALLENGE link whose target claim was created by the
// conceding user (highest link id wins if more than one matches).
int resolveChallengeLinkIdForUser(DebateWrapper& debateWrapper, int debate_id, int conceding_user_id) {
    int bestFromClaimId = -1;
    int bestLinkId = -1;
    auto allLinks = debateWrapper.getLinksForDebate(debate_id);
    const std::set<int> concededRelations =
        ConcededRelationsByUser(debateWrapper, debate_id, conceding_user_id);

    for (const auto& linkTuple : allLinks) {
        int linkId = std::get<0>(linkTuple);
        int fromClaimId = std::get<1>(linkTuple);
        int toClaimId = std::get<2>(linkTuple);
        int linkType = std::get<5>(linkTuple);
        if (linkType != static_cast<int>(debate::LinkType::CHALLENGE)) continue;
        debate::Claim targetClaim = debateWrapper.getClaimById(toClaimId);
        if (targetClaim.id() == 0 || targetClaim.creator_id() != conceding_user_id) continue;

        // Skip what this user already conceded -- otherwise repeated
        // concessions all resolve to the same challenge. Read off the move
        // log, which is what records a concession. Mirrors the runner.
        if (concededRelations.count(linkId)) {
            continue;
        }

        if (fromClaimId > bestFromClaimId) {
            bestFromClaimId = fromClaimId;
            bestLinkId = linkId;
        }
    }
    return bestLinkId;
}

// A small importer that walks a TestScenario and executes each action against
// a live DebateModerator, mirroring ScenarioRunner::forgeEvent()/executeScenario()
// from the gtest-based scenario runner — minus expectation checking.
class ScenarioImporterImpl {
public:
    explicit ScenarioImporterImpl(DebateModerator& moderator) : moderator_(moderator) {}

    // Returns true if every step executed successfully.
    bool Run(const TestScenario& scenario) {
        int step_index = 0;
        for (const auto& step : scenario.steps()) {
            if (step.has_action()) {
                if (!RunAction(step.action(), step_index)) {
                    std::cerr << "[scenario] FAILED at step " << step_index << std::endl;
                    return false;
                }
            }
            step_index++;
        }
        std::cout << "[scenario] Imported " << step_index << " step(s) successfully."
                   << " debate_id=" << debate_id_ << std::endl;
        return true;
    }

    // Replays one action outside a scenario loop -- the tutorial's step-per-
    // click path. Same function the loop calls, so the two cannot drift.
    bool RunOne(const TestAction& action, int step_index) {
        return RunAction(action, step_index);
    }

    // Makes a scenario name resolve to an existing account. See
    // Importer::AliasUser.
    void SetAlias(const std::string& script_name, const std::string& real_username) {
        user_aliases_[script_name] = real_username;
    }

    // Rebuilds the id maps for a debate an earlier importer built. See
    // Importer::Attach for why sorting by claim id reproduces the numbering.
    void AttachTo(int real_debate_id) {
        debate_id_ = real_debate_id;
        scenario_to_real_debate_[1] = real_debate_id;
        scenario_to_real_claim_.clear();
        next_scenario_claim_id_ = 1;
        // ClaimIdsInDebate returns a std::set, so this is already ascending --
        // which is creation order, which is the order a scenario numbers them.
        for (const int real_id : ClaimIdsInDebate(real_debate_id)) {
            scenario_to_real_claim_[next_scenario_claim_id_++] = real_id;
        }
    }

private:
    // A scenario name that has been pointed at a real account, e.g.
    // "YOU" -> "guest". Applied to the username BEFORE anything else looks at
    // it, so the forged event, the created user, and the concede resolution all
    // agree on who acted. Aliasing by user id alone was not enough: the event
    // carries a username too, and the two disagreeing produces a debate whose
    // moves are attributed to a user who never existed.
    std::string ResolveUsername(const std::string& script_name) const {
        auto it = user_aliases_.find(script_name);
        return it == user_aliases_.end() ? script_name : it->second;
    }

    int GetUserId(const std::string& username) {
        auto it = user_ids_.find(username);
        if (it != user_ids_.end()) return it->second;
        int id = moderator_.createUserIfNotExist(username);
        user_ids_[username] = id;
        return id;
    }

    bool RunAction(const TestAction& raw_action, int step_index) {
        // Substitute aliases once, here, so every later read of username() --
        // event forging, user creation, concede resolution -- sees the same
        // name.
        TestAction action = raw_action;
        action.set_username(ResolveUsername(raw_action.username()));

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

        // The page for each step would be thrown away; whoever drives the
        // importer answers with one page at the end.
        moderator_.apply(event);

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
                if (!action.voice().empty()) {
                    event.mutable_create_debate()->set_voice(action.voice());
                }
                if (!action.source_url().empty()) {
                    event.mutable_create_debate()->set_source_url(action.source_url());
                }
                if (!action.source_excerpt().empty()) {
                    event.mutable_create_debate()->set_source_excerpt(action.source_excerpt());
                }
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
            // Selects a claim without leaving the graph view, where GO_TO_CLAIM
            // drops you into the single-claim page. A walkthrough needs to point
            // at a claim while the reader keeps looking at the whole argument,
            // and without this case the id fell through to `default` and was
            // silently dropped -- the event still fired, just at claim 0.
            case debate_event::SELECT_CLAIM:
                event.mutable_select_claim()->set_claim_id(RealClaimId(action.claim_id()));
                break;
            // Challenge an ARGUMENT. The scenario names the link by its ends
            // (the claim it is for, the claim offered for it); the stored link
            // is looked up here, since ids differ between databases.
            case debate_event::ADD_LINK_TO_BE_CHALLENGED: {
                const int parent = RealClaimId(action.link_to_claim_id());
                const int child = RealClaimId(action.link_from_claim_id());
                int linkId = 0;
                DebateWrapper& dw = moderator_.getDebateWrapper();
                for (int id : dw.findLinksUnder(parent)) {
                    const debate::Relationship::Link l = dw.getLinkById(id).link();
                    if (l.connect_from() == parent && l.connect_to() == child) { linkId = id; break; }
                }
                event.mutable_add_link_to_be_challenged()->set_link_id(linkId);
                break;
            }
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
                    // A scenario has one claim_description field, so the extras
                    // get none. Padded rather than omitted so the two repeated
                    // fields stay index-aligned on the server.
                    event.mutable_add_child_claim()->add_extra_descriptions("");
                }
                if (!action.voice().empty()) {
                    event.mutable_add_child_claim()->set_voice(action.voice());
                }
                if (!action.source_url().empty()) {
                    event.mutable_add_child_claim()->set_source_url(action.source_url());
                }
                if (!action.source_excerpt().empty()) {
                    event.mutable_add_child_claim()->set_source_excerpt(action.source_excerpt());
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
                    event.mutable_submit_challenge_claim()->add_extra_descriptions("");
                }
                if (!action.voice().empty()) {
                    event.mutable_submit_challenge_claim()->set_voice(action.voice());
                }
                if (!action.source_url().empty()) {
                    event.mutable_submit_challenge_claim()->set_source_url(action.source_url());
                }
                if (!action.source_excerpt().empty()) {
                    event.mutable_submit_challenge_claim()->set_source_excerpt(action.source_excerpt());
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
                if (!action.voice().empty()) {
                    std::cout << " (said by " << action.voice() << ")";
                }
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
    std::map<std::string, std::string> user_aliases_;
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

public:
    // The debate this run created. Read by scenario::Importer after Run.
    int created_debate_id() const { return debate_id_; }

private:
    std::map<int, int> scenario_to_real_debate_;
    std::map<int, int> scenario_to_real_claim_;
    int next_scenario_claim_id_ = 1;
};
} // namespace

namespace scenario {

TestScenario LoadFromText(const std::string& text, bool& ok, std::string& error) {
    TestScenario scenario;
    // A model wrote this text, so a parse failure is an expected outcome and
    // the reason has to reach the user rather than a log nobody reads.
    if (!google::protobuf::TextFormat::ParseFromString(stripCodeFence(text), &scenario)) {
        ok = false;
        error = "The generated scenario was not valid protobuf text format.";
        return scenario;
    }
    if (scenario.steps_size() == 0) {
        ok = false;
        error = "The generated scenario has no steps.";
        return scenario;
    }
    ok = true;
    return scenario;
}

TestScenario LoadFromFile(const std::string& path, bool& ok) {
    return LoadScenarioFromFile(path, ok);
}

struct Importer::State {
    explicit State(DebateModerator& moderator) : impl(moderator) {}
    ScenarioImporterImpl impl;
    int steps = 0;
};

Importer::Importer(DebateModerator& moderator) : state_(new State(moderator)) {}
Importer::~Importer() { delete state_; }

bool Importer::Run(const TestScenario& scenario) {
    state_->steps = scenario.steps_size();
    return state_->impl.Run(scenario);
}

bool Importer::RunAction(const TestAction& action) {
    // Step index is only ever printed, and a tutorial's steps arrive one per
    // request with no loop counter to hand. Counting them here keeps the log
    // readable without inventing a number the caller would have to carry.
    return state_->impl.RunOne(action, state_->steps++);
}

void Importer::Attach(int real_debate_id) { state_->impl.AttachTo(real_debate_id); }

void Importer::AliasUser(const std::string& script_name, const std::string& real_username) {
    state_->impl.SetAlias(script_name, real_username);
}

int Importer::debate_id() const { return state_->impl.created_debate_id(); }
int Importer::steps_run() const { return state_->steps; }

} // namespace scenario
