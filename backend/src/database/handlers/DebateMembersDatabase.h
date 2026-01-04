#pragma once

#include <string>
#include <vector>
#include "../sqlite/Database.h"

// ---------------------------------------
// Class: DebateMembersDatabase
// ---------------------------------------
// Junction table for many-to-many relationship
// between debates and users using the Database wrapper class.
// ---------------------------------------
class DebateMembersDatabase {
public:
    // Constructor takes a reference to the Database instance
    explicit DebateMembersDatabase(Database& db);

    bool ensureTable();
    bool addMember(int debateId, int userId);
    std::vector<int> getDebateIdsForUser(int userId);
    std::vector<int> getUserIdsForDebate(int debateId);
    bool isMember(int debateId, int userId);
    bool removeMember(int debateId, int userId);
    bool removeAllMembersFromDebate(int debateId);
    bool removeUserFromAllDebates(int userId);

private:
    Database& db_;
};
