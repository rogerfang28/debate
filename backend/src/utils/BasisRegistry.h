#pragma once

#include "basis.pb.h"

#include <string>
#include <vector>

// The one place the code learns what kinds of reason exist.
//
// Reads backend/config/bases.pbtxt (or DEBATE_BASES_FILE) and answers the
// four questions the rest of the code used to answer from string literals:
// what the names are, which is the default, how each one propagates, and how
// to describe them to a model. Nothing outside this file should spell a basis
// name.
//
// The file is re-read when it changes -- checked by modification time and
// size on each access -- so an edit shows up in the next dropdown and the
// next status computation without a restart, and a status recompute that
// asks about thirty links does not parse the file thirty times.
namespace bases {

// Where the registry is read from: DEBATE_BASES_FILE if set, else the first
// of a few locations relative to the executable that exists. The candidates
// differ because the server and the test binaries sit at different depths
// from backend/config.
std::string FilePath();

// The parsed registry. Empty -- and logged, once per file state -- if the
// file is missing or does not parse. An empty registry is survivable: the
// dropdowns show nothing, the prompts say so, writes carry no basis, and
// propagation falls back to COLLAPSE, which is what every link did before the
// registry existed.
const basis::BasisRegistry& Registry();

// Names in file order. This is the dropdown.
std::vector<std::string> Names();

// The entry marked is_default, else the first entry, else "".
std::string DefaultName();

bool IsKnown(const std::string& name);

// `name` if the registry knows it, otherwise "" with a warning. The write
// path runs every basis through this, so a stored basis is always one the
// registry can explain. Empty in, empty out, silently: unspecified is
// allowed and is not the same as unknown.
std::string Sanitize(const std::string& name);

// How a link of this kind carries status. Unknown, empty, and the two
// placeholder strings written before bases existed all get COLLAPSE, because
// that is the behaviour those links have always had.
basis::Propagation PropagationFor(const std::string& name);

// The registry explained to a model, one basis per line:
//   NAME -- description. Supporting: ... Challenging: ...
// Empty registry -> a line saying no kinds are configured, so a prompt never
// silently omits the section.
std::string GuideAsList();

// The same, as a single sentence fragment for a prompt that lists the
// options inline: NAME (as_challenge), NAME (as_challenge), ...
// Uses the challenge gloss because the one caller that wants this shape is
// asking the model to challenge.
std::string GuideInline();

// Drops the cached parse so the next access re-reads. Tests use it after
// pointing DEBATE_BASES_FILE somewhere else; production never needs it.
void ResetCache();

} // namespace bases
