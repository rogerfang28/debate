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
    bool addMember(const std::string& debateId, int userId);
    std::vector<std::string> getDebateIdsForUser(int userId);
    std::vector<int> getUserIdsForDebate(const std::string& debateId);
    bool isMember(const std::string& debateId, int userId);
    bool removeMember(const std::string& debateId, int userId);
    bool removeAllMembersFromDebate(const std::string& debateId);
    bool removeUserFromAllDebates(int userId);

private:
    Database& db_;
};
