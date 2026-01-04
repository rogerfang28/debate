#pragma once

#include "../sqlite/Database.h"
#include "UserDatabase.h"
#include "DebateDatabase.h"
#include "StatementDatabase.h"
#include "ChallengeDatabase.h"
#include "DebateMembersDatabase.h"
#include "LinkDatabase.h"

// ---------------------------------------
// Class: DatabaseWrapper
// ---------------------------------------
// Simple wrapper that contains all database handlers
// and provides access to their functions.
// ---------------------------------------
class DatabaseWrapper {
public:
    // Constructor takes a reference to the Database instance
    explicit DatabaseWrapper(Database& db)
        : users(db),
          debates(db),
          statements(db),
          challenges(db),
          debateMembers(db),
          links(db) {}

    // Initialize all tables
    bool ensureAllTables() {
        return users.ensureTable() &&
               debates.ensureTable() &&
               statements.ensureTable() &&
               challenges.ensureTable() &&
               debateMembers.ensureTable() &&
               links.ensureTable();
    }

    // Public database handlers - can call any of their functions
    UserDatabase users;
    DebateDatabase debates;
    StatementDatabase statements;
    ChallengeDatabase challenges;
    DebateMembersDatabase debateMembers;
    LinkDatabase links;
};
