#pragma once

#include "test_scenario.pb.h"
#include <string>

class DebateModerator;

// Replays a scenario -- a list of user actions -- against a live moderator,
// creating a real debate.
//
// This was written as a test tool and lives here now because AI generation
// uses the same path: the model writes a scenario, and this replays it. Two
// copies of "turn a scenario into a debate" would drift, and the drift would
// show up as generated debates behaving unlike imported ones.
//
// Every step goes through DebateModerator::handleRequest, so a replayed debate
// is indistinguishable from a typed one -- same events, same move log, same
// status computation.
//
// The id remapping is the subtle part. A scenario says `debate_id: 1` because
// a test database is empty and the first debate really is 1. A live database
// already has debates, so the ids are translated as the scenario runs.
// Without that, an import silently grafts itself onto somebody else's debate.
namespace scenario {

// Parses protobuf text format. `ok` is false if the text is not a valid
// debate_test::TestScenario -- which is the normal outcome when a model wrote it.
debate_test::TestScenario LoadFromText(const std::string& text, bool& ok, std::string& error);
debate_test::TestScenario LoadFromFile(const std::string& path, bool& ok);

class Importer {
public:
    explicit Importer(DebateModerator& moderator);
    ~Importer();

    // True if every step executed. Stops at the first failure rather than
    // pressing on, so a half-built debate is not mistaken for a whole one.
    bool Run(const debate_test::TestScenario& scenario);

    // Replays ONE action rather than a whole scenario. The tutorial advances a
    // step per click, so its steps arrive across separate requests instead of
    // in a single loop -- but they must go through exactly the same code, or a
    // walkthrough would teach behaviour imports do not have.
    bool RunAction(const debate_test::TestAction& action);

    // Re-points this importer at a debate an earlier importer built, rebuilding
    // the scenario->real id maps from what is in the database.
    //
    // This is what lets a walkthrough survive a server restart, and it is why
    // no tutorial state is held in memory between clicks. Claims are numbered
    // by a scenario from 1 in creation order, and a debate's claim ids ascend
    // in creation order too, so sorting the debate's claims reproduces the map
    // exactly.
    void Attach(int real_debate_id);

    // Makes `script_name` in a scenario mean an existing real user.
    //
    // A tutorial is watched by a real person, so the actions attributed to
    // "YOU" have to be performed BY them -- otherwise the walkthrough builds a
    // debate between two strangers while the reader watches from outside it,
    // and none of the statuses on screen are theirs.
    void AliasUser(const std::string& script_name, const std::string& real_username);

    // The debate this run created, once it has created one.
    int debate_id() const;
    int steps_run() const;

private:
    struct State;
    State* state_;
};

} // namespace scenario
