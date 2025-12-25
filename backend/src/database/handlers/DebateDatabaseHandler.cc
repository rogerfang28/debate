// ! need to fully understand

#include "DebateDatabaseHandler.h"
#include "../../utils/Log.h"

DebateDatabaseHandler::DebateDatabaseHandler(const std::string& dbFile)
    : dbFilename(dbFile) {
    ensureTable();
}

void DebateDatabaseHandler::ensureTable() {
    if (!openDB(dbFilename)) return;
    createTable("DEBATE", {
        {"USER", "TEXT NOT NULL"},
        {"TOPIC", "TEXT NOT NULL"},
        {"PAGE_DATA","BLOB"} // binary protobuf
    });
    closeDB();
}

int DebateDatabaseHandler::addDebate(const std::string& user, const std::string& topic) {
    if (!openDB(dbFilename)) return -1;
    int id = insertRowWithText("DEBATE", {"USER", "TOPIC"}, {user, topic});
    closeDB();
    if (id != -1)
        Log::debug("[DebateDB] Added debate: " + topic + " for user " + user);
    else {
        Log::warn("[DebateDB] Failed to add debate: " + topic + " for user " + user);
    }
    return id;
}

int DebateDatabaseHandler::addDebateWithProtobuf(const std::string& user, const std::string& topic, 
                                                 const std::vector<uint8_t>& protobufData) {
    if (!openDB(dbFilename)) return -1;
    
    std::vector<std::pair<const void*, int>> blobValues;
    if (!protobufData.empty()) {
        blobValues.push_back({protobufData.data(), static_cast<int>(protobufData.size())});
    } else {
        blobValues.push_back({nullptr, 0});
    }
    
    int id = insertRowWithBlob("DEBATE", {"USER", "TOPIC", "PAGE_DATA"}, 
                               {user, topic}, blobValues);
    closeDB();
    if (id != -1)
        Log::debug("[DebateDB] Added debate with protobuf: " + topic + " for user " + user 
                  + " (data size: " + std::to_string(protobufData.size()) + " bytes)");
    return id;
}

std::vector<std::map<std::string, std::string>>
DebateDatabaseHandler::getDebates(const std::string& user) {
    if (!openDB(dbFilename)) return {};
    auto rows = readRows("DEBATE", "USER = '" + user + "'");
    closeDB();
    Log::debug("[DebateDB] Retrieved " + std::to_string(rows.size())
              + " debates for user " + user);
    return rows;
}

std::vector<uint8_t> DebateDatabaseHandler::getDebateProtobuf(const std::string& id) {
    if (!openDB(dbFilename)) return {};
    
    std::string whereClause = "ID = '" + id + "'";
    auto protobufData = readBlob("DEBATE", "PAGE_DATA", whereClause);
    closeDB();
    
    // std::cout << "[DebateDB] Retrieved protobuf data for id: " << id 
            // << " (size: " << protobufData.size() << " bytes)\n";
    return protobufData;
}

bool DebateDatabaseHandler::updateDebateProtobuf(const std::string& user, const std::string& id, 
                                                  const std::vector<uint8_t>& protobufData) {
    Log::debug("[DebateDB] Updating protobuf data for id: " + id 
              + " for user " + user + " (size: " + std::to_string(protobufData.size()) + " bytes)");
    if (!openDB(dbFilename)) return false;
    
    std::string whereClause = "USER = '" + user + "' AND ID = '" + id + "'";
    bool success = updateRowWithBlob("DEBATE", "PAGE_DATA", protobufData, whereClause);
    closeDB();
    
    if (success) {
        Log::debug("[DebateDB] Updated protobuf data for id: " + id 
                  + " for user " + user + " (size: " + std::to_string(protobufData.size()) + " bytes)");
    }
    return success;
}

bool DebateDatabaseHandler::removeDebate(const std::string& id, const std::string& user) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("DEBATE", "ID = '" + id + "' AND USER = '" + user + "'");
    closeDB();
    if (ok)
        // std::cout << "[DebateDB] Removed debate with id: " << id << " for user " << user << "\n";
    return ok;
}

bool DebateDatabaseHandler::clearUserDebates(const std::string& user) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("DEBATE", "USER = '" + user + "'");
    closeDB();
    if (ok)
        Log::debug("[DebateDB] Cleared all debates for user: " + user);
    return ok;
}

bool DebateDatabaseHandler::debateExists(const std::string& id) {
    if (!openDB(dbFilename)) return false;
    auto rows = readRows("DEBATE", "ID = '" + id + "'");
    closeDB();
    bool exists = !rows.empty();
    Log::debug("[DebateDB] Debate with id: " + id 
              + (exists ? " exists." : " does not exist."));
    return exists;
}