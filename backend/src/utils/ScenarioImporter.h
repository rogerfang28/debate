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

    // The debate this run created, once it has created one.
    int debate_id() const;
    int steps_run() const;

private:
    struct State;
    State* state_;
};

} // namespace scenario
