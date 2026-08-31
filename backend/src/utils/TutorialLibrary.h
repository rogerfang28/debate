#pragma once

#include "tutorial.pb.h"

#include <string>
#include <vector>

// Finds and loads the walkthrough scripts in backend/tutorials/.
//
// Nothing is cached. A tutorial is read from disk every time it is needed,
// which costs a few kilobytes on a button click and buys the thing these files
// exist for: edit a .pbtxt, refresh the page, read the new words. Caching
// would mean every wording change needed a server restart, and a walkthrough
// nobody can revise quickly is one that stays wrong.
//
// Adding a walkthrough of another case -- the Fauci one, say -- is a new file
// in that directory. No code here knows the name of any particular tutorial.
namespace tutorials {

// Where the scripts live. Override with DEBATE_TUTORIALS_DIR.
std::string Directory();

struct Listing {
    std::string name;      // filename stem, the id used everywhere else
    std::string title;
    std::string subtitle;
    int order = 0;
};

// Every readable script, ordered by each script's own `order` field and then
// by filename. Unparseable files are skipped rather than throwing -- one bad
// file should not take the home page down with it.
std::vector<Listing> List();

// The walkthrough offered by default: the one named by DEBATE_TUTORIAL, or
// else the first one List() returns. Empty if the directory has none.
std::string DefaultName();

// Loads one script. `ok` is false and `error` says why if it is missing or
// does not parse -- both are ordinary outcomes when the files are meant to be
// hand-edited.
tutorial::Tutorial Load(const std::string& name, bool& ok, std::string& error);

} // namespace tutorials
