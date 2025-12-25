#pragma once

#include <string>
#include <vector>
#include <iostream>

// ---------------------------------------
// Class: DebateMembersDatabaseHandler
// ---------------------------------------
// Junction table for many-to-many relationship
// between debates and users
// ---------------------------------------
class DebateMembersDatabaseHandler {
public:
    explicit DebateMembersDatabaseHandler(const std::string& dbFile);
    ~DebateMembersDatabaseHandler() = default;
    void ensureTable();
    int addMember(const std::string& debateId, const std::string& userId);
    std::vector<std::string> getDebateIdsForUser(const std::string& userId) const;
    std::vector<std::string> getUserIdsForDebate(const std::string& debateId) const;
    bool isMember(const std::string& debateId, const std::string& userId) const;
    bool removeMember(const std::string& debateId, const std::string& userId);
    bool removeAllMembersFromDebate(const std::string& debateId);
    bool removeUserFromAllDebates(const std::string& userId);

private:
    std::string dbFilename;
};
