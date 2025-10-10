#include "DebateDatabaseHandler.h"

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
        std::cout << "[DebateDB] Added debate: " << topic << " for user " << user << "\n";
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
        std::cout << "[DebateDB] Added debate with protobuf: " << topic << " for user " << user 
                  << " (data size: " << protobufData.size() << " bytes)\n";
    return id;
}

std::vector<std::map<std::string, std::string>>
DebateDatabaseHandler::getDebates(const std::string& user) {
    if (!openDB(dbFilename)) return {};
    auto rows = readRows("DEBATE", "USER = '" + user + "'");
    closeDB();
    std::cout << "[DebateDB] Retrieved " << rows.size()
              << " debates for user " << user << "\n";
    return rows;
}

std::vector<uint8_t> DebateDatabaseHandler::getDebateProtobuf(const std::string& user, 
                                                              const std::string& topic) {
    if (!openDB(dbFilename)) return {};
    
    std::string whereClause = "USER = '" + user + "' AND TOPIC = '" + topic + "'";
    auto protobufData = readBlob("DEBATE", "PAGE_DATA", whereClause);
    closeDB();
    
    std::cout << "[DebateDB] Retrieved protobuf data for topic: " << topic 
              << " for user " << user << " (size: " << protobufData.size() << " bytes)\n";
    return protobufData;
}

bool DebateDatabaseHandler::updateDebateProtobuf(const std::string& user, const std::string& topic, 
                                                  const std::vector<uint8_t>& protobufData) {
    std::cout << "[DebateDB] Updating protobuf data for topic: " << topic 
              << " for user " << user << " (size: " << protobufData.size() << " bytes)\n";
    if (!openDB(dbFilename)) return false;
    
    std::string whereClause = "USER = '" + user + "' AND TOPIC = '" + topic + "'";
    bool success = updateRowWithBlob("DEBATE", "PAGE_DATA", protobufData, whereClause);
    closeDB();
    
    if (success) {
        std::cout << "[DebateDB] Updated protobuf data for topic: " << topic 
                  << " for user " << user << " (size: " << protobufData.size() << " bytes)\n";
    }
    return success;
}

bool DebateDatabaseHandler::removeDebate(const std::string& topic, const std::string& user) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("DEBATE", "TOPIC = '" + topic + "' AND USER = '" + user + "'");
    closeDB();
    if (ok)
        std::cout << "[DebateDB] Removed debate: " << topic << " for user " << user << "\n";
    return ok;
}

bool DebateDatabaseHandler::clearUserDebates(const std::string& user) {
    if (!openDB(dbFilename)) return false;
    bool ok = deleteRows("DEBATE", "USER = '" + user + "'");
    closeDB();
    if (ok)
        std::cout << "[DebateDB] Cleared all debates for user: " << user << "\n";
    return ok;
}
