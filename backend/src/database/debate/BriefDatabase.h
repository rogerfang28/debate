#pragma once

#include <string>
#include "../sqlite/Database.h"

// ---------------------------------------
// BriefDatabase
// ---------------------------------------
// The words of a debate's Brief tab: one row per debate.
//
// The brief's STRUCTURE (who holds what, which challenges are open, whose
// move it is) is computed from the debate on every page build and is never
// stored. What is stored is the prose the model wrote for that structure --
// the verdict, the story, the name of each battle site -- together with a
// fingerprint of the structure it was written for. A reader whose debate has
// moved on since then is told the words are behind, not shown stale ones as
// current.
//
// One row per debate, not per reader: the words are about the debate, so
// every reader shares them, and one model run serves everybody.
// ---------------------------------------
class BriefDatabase {
public:
    explicit BriefDatabase(Database& db);

    bool ensureTable();

    // Stores (or replaces) the brief for one debate.
    bool save(int debateId, const std::string& fingerprint, const std::string& text);

    // Loads the brief for one debate. Returns false when there is none.
    bool load(int debateId, std::string* fingerprint, std::string* text);

private:
    Database& db_;
};
