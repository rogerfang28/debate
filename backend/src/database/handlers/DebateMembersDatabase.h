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
    bool addMember(const std::string& debateId, const std::string& userId);
    std::vector<std::string> getDebateIdsForUser(const std::string& userId);
    std::vector<std::string> getUserIdsForDebate(const std::string& debateId);
    bool isMember(const std::string& debateId, const std::string& userId);
    bool removeMember(const std::string& debateId, const std::string& userId);
    bool removeAllMembersFromDebate(const std::string& debateId);
    bool removeUserFromAllDebates(const std::string& userId);

private:
    Database& db_;
};
