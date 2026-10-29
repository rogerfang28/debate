#pragma once

#include "test_scenario.pb.h"

#include <cstddef>
#include <string>
#include <vector>

// Reads the text an admin pastes into "Import debate" and turns it into the
// scenario steps scenario::Importer replays. Nothing here touches a database:
// Preview runs this alone, and Publish runs it again before replaying.
//
// Two formats, told apart automatically:
//
//   scenario  debate_test::TestScenario in protobuf text format -- what
//             test/scenarios/generators/gen_*_sourced.py write. Checked step
//             by step so that an import cannot reach outside the debate it
//             creates (see ReadScenario).
//
//   outline   one claim per line, written by hand (docs/IMPORT_FORMAT.md):
//
//               Root claim | Voice | https://source | "quote" | BASIS | note
//                 + supports the line above it | ...
//                   - challenges the line above it | ...
//
//             The parent of a + or - line is the nearest earlier line with
//             less indentation (a tab counts as 4 spaces). Converted into the
//             same steps the generators emit.
namespace debate_import {

constexpr std::size_t kMaxBytes = 300 * 1024;
constexpr int kMaxClaims = 500;

// The acting user of the steps an outline becomes. The caller aliases it to
// the importing admin, exactly as it aliases a scenario's own acting user.
constexpr const char* kOutlineActor = "guest";

struct Plan {
    std::string format;  // "scenario" | "outline" | "" (nothing to read)
    debate_test::TestScenario scenario;
    // The one username the scenario's steps act as.
    std::string actor;
    std::string root;
    int claims = 0;  // the root included
    int supports = 0;
    int challenges = 0;
    // Named voices, in first-use order, each once. A claim with no voice is
    // the importer's own and is not listed.
    std::vector<std::string> voices;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    // For each step of `scenario`, the outline line it came from (1-based);
    // 0 for a scenario file.
    std::vector<int> stepLines;

    bool ok() const { return errors.empty(); }
};

// Detects the format and reads the text.
Plan Read(const std::string& text);

// Each format on its own (Read picks one). Exposed for tests.
Plan ReadOutline(const std::string& text);
Plan ReadScenario(const std::string& text);

// Whether text that did not parse as a scenario was meant to be one: its first
// line that is not blank or a # comment starts with `name:` or `steps`.
bool LooksLikeScenario(const std::string& text);

}  // namespace debate_import
