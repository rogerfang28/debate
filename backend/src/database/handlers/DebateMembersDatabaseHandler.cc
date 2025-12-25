#include "DebateMembersDatabaseHandler.h"
#include <iostream>
#include <vector>
#include "../sqlite/databaseCommunicator.h"
#include "../../utils/Log.h"

DebateMembersDatabaseHandler::DebateMembersDatabaseHandler(const std::string& dbFile)
    : dbFilename(dbFile) {
    ensureTable();
}

void DebateMembersDatabaseHandler::ensureTable() {
    if (!openDB(dbFilename)) return;

    createTable("DEBATE_MEMBERS", {
        {"DEBATE_ID", "TEXT NOT NULL"},
        {"USER_ID", "TEXT NOT NULL"}
    });

    closeDB();
}

int DebateMembersDatabaseHandler::addMember(const std::string& debateId, const std::string& userId) {
    if (!openDB(dbFilename)) return -1;

    int id = insertRowWithText("DEBATE_MEMBERS", {"DEBATE_ID", "USER_ID"}, {debateId, userId});
    closeDB();

    if (id != -1)
        Log::debug("[DebateMembersDB] Added user " + userId 
                  + " to debate " + debateId);

    return id;
}

std::vector<std::string> DebateMembersDatabaseHandler::getDebateIdsForUser(const std::string& userId) const {
    if (!openDB(dbFilename)) return {};

    auto rows = readRows("DEBATE_MEMBERS", "USER_ID = '" + userId + "'");
    closeDB();

    std::vector<std::string> debateIds;
    for (const auto& row : rows) {
        if (row.count("DEBATE_ID")) {
            debateIds.push_back(row.at("DEBATE_ID"));
        }
    }

    Log::debug("[DebateMembersDB] Found " + std::to_string(debateIds.size()) 
              + " debates for user " + userId);
    return debateIds;
}

std::vector<std::string> DebateMembersDatabaseHandler::getUserIdsForDebate(const std::string& debateId) const {
    if (!openDB(dbFilename)) return {};

    auto rows = readRows("DEBATE_MEMBERS", "DEBATE_ID = '" + debateId + "'");
    closeDB();

    std::vector<std::string> userIds;
    for (const auto& row : rows) {
        if (row.count("USER_ID")) {
            userIds.push_back(row.at("USER_ID"));
        }
    }

    Log::debug("[DebateMembersDB] Found " + std::to_string(userIds.size()) 
              + " users in debate " + debateId);
    return userIds;
}

bool DebateMembersDatabaseHandler::isMember(const std::string& debateId, const std::string& userId) const {
    if (!openDB(dbFilename)) return false;

    auto rows = readRows("DEBATE_MEMBERS", 
                        "DEBATE_ID = '" + debateId + "' AND USER_ID = '" + userId + "'");
    closeDB();

    return !rows.empty();
}

bool DebateMembersDatabaseHandler::removeMember(const std::string& debateId, const std::string& userId) {
    if (!openDB(dbFilename)) return false;

    bool ok = deleteRows("DEBATE_MEMBERS", 
                        "DEBATE_ID = '" + debateId + "' AND USER_ID = '" + userId + "'");
    closeDB();

    if (ok)
        Log::debug("[DebateMembersDB] Removed user " + userId 
                  + " from debate " + debateId);
    return ok;
}

bool DebateMembersDatabaseHandler::removeAllMembersFromDebate(const std::string& debateId) {
    if (!openDB(dbFilename)) return false;

    bool ok = deleteRows("DEBATE_MEMBERS", "DEBATE_ID = '" + debateId + "'");
    closeDB();

    if (ok)
        // std::cout << "[DebateMembersDB] Removed all members from debate " << debateId << "\n";
    return ok;
}

bool DebateMembersDatabaseHandler::removeUserFromAllDebates(const std::string& userId) {
    if (!openDB(dbFilename)) return false;

    bool ok = deleteRows("DEBATE_MEMBERS", "USER_ID = '" + userId + "'");
    closeDB();

    if (ok)
        Log::debug("[DebateMembersDB] Removed user " + userId + " from all debates");
    return ok;
}
