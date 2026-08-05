#pragma once

#include "../sqlite/Database.h"
#include "UserDatabase.h"
#include "DebateDatabase.h"
#include "StatementDatabase.h"
#include "DebateMembersDatabase.h"
#include "LinkDatabase.h"
#include "MoveDatabase.h"

// ---------------------------------------
// Class: DatabaseWrapper
// ---------------------------------------
// Simple wrapper that contains all database handlers
// and provides access to their functions.
// ---------------------------------------
class DatabaseWrapper {
public:
    // Debate data (debates/statements/members/links) lives in the debates DB;
    // user records live in a separate users DB (users.sqlite3) so there is a
    // single source of truth for users shared with the VirtualRenderer.
    DatabaseWrapper(Database& debatesDb, Database& usersDb)
        : users(usersDb),
          debates(debatesDb),
          statements(debatesDb),
          debateMembers(debatesDb),
          links(debatesDb),
          moves(debatesDb) {}

    // Initialize all tables
    bool ensureAllTables() {
        return users.ensureTable() &&
               debates.ensureTable() &&
               statements.ensureTable() &&
               debateMembers.ensureTable() &&
               links.ensureTable() &&
               moves.ensureTable();
    }

    // Public database handlers - can call any of their functions
    UserDatabase users;
    DebateDatabase debates;
    StatementDatabase statements;
    DebateMembersDatabase debateMembers;
    LinkDatabase links;
    // Append-only action log. Parallel record only -- nothing reads it yet.
    MoveDatabase moves;
};
