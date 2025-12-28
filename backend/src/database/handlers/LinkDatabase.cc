#include "LinkDatabase.h"
#include <iostream>
#include "../../utils/Log.h"

LinkDatabase::LinkDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool LinkDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS LINKS (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            CLAIM_ID_FROM INTEGER NOT NULL,
            CLAIM_ID_TO INTEGER NOT NULL,
            CONNECTION TEXT NOT NULL,
            CREATOR TEXT NOT NULL
        );
    )";
    return db_.execute(sql);
}

int LinkDatabase::addLink(int claimIdFrom, int claimIdTo, const std::string& connection, const std::string& creator) {
    const char* sql = "INSERT INTO LINKS (CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR) VALUES (?, ?, ?, ?);";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, claimIdFrom);
    sqlite3_bind_int(stmt, 2, claimIdTo);
    sqlite3_bind_text(stmt, 3, connection.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, creator.c_str(), -1, SQLITE_TRANSIENT);

    int result = sqlite3_step(stmt);
    int linkId = -1;
    
    if (result == SQLITE_DONE) {
        linkId = static_cast<int>(sqlite3_last_insert_rowid(db_.handle()));
    } else {
        Log::error("Failed to insert link: " + std::string(sqlite3_errmsg(db_.handle())));
    }

    sqlite3_finalize(stmt);
    return linkId;
}

std::vector<std::tuple<int, int, int, std::string, std::string>> LinkDatabase::getLinksFromClaim(int claimIdFrom) {
    std::vector<std::tuple<int, int, int, std::string, std::string>> links;
    const char* sql = "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR FROM LINKS WHERE CLAIM_ID_FROM = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return links;
    }

    sqlite3_bind_int(stmt, 1, claimIdFrom);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int fromId = sqlite3_column_int(stmt, 1);
        int toId = sqlite3_column_int(stmt, 2);
        const char* connection = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* creator = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        
        links.push_back(std::make_tuple(id, fromId, toId, connection ? connection : "", creator ? creator : ""));
    }

    sqlite3_finalize(stmt);
    return links;
}

std::vector<std::tuple<int, int, int, std::string, std::string>> LinkDatabase::getLinksToClaim(int claimIdTo) {
    std::vector<std::tuple<int, int, int, std::string, std::string>> links;
    const char* sql = "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR FROM LINKS WHERE CLAIM_ID_TO = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return links;
    }

    sqlite3_bind_int(stmt, 1, claimIdTo);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int fromId = sqlite3_column_int(stmt, 1);
        int toId = sqlite3_column_int(stmt, 2);
        const char* connection = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* creator = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        
        links.push_back(std::make_tuple(id, fromId, toId, connection ? connection : "", creator ? creator : ""));
    }

    sqlite3_finalize(stmt);
    return links;
}

std::vector<std::tuple<int, int, int, std::string, std::string>> LinkDatabase::getLinksForClaim(int claimId) {
    std::vector<std::tuple<int, int, int, std::string, std::string>> links;
    const char* sql = "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR FROM LINKS WHERE CLAIM_ID_FROM = ? OR CLAIM_ID_TO = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return links;
    }

    sqlite3_bind_int(stmt, 1, claimId);
    sqlite3_bind_int(stmt, 2, claimId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int fromId = sqlite3_column_int(stmt, 1);
        int toId = sqlite3_column_int(stmt, 2);
        const char* connection = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* creator = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        
        links.push_back(std::make_tuple(id, fromId, toId, connection ? connection : "", creator ? creator : ""));
    }

    sqlite3_finalize(stmt);
    return links;
}

std::optional<std::tuple<int, int, int, std::string, std::string>> LinkDatabase::getLinkById(int linkId) {
    const char* sql = "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR FROM LINKS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return std::nullopt;
    }

    sqlite3_bind_int(stmt, 1, linkId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int fromId = sqlite3_column_int(stmt, 1);
        int toId = sqlite3_column_int(stmt, 2);
        const char* connection = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const char* creator = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        
        // Convert to std::string BEFORE finalizing the statement
        std::string connectionStr = connection ? std::string(connection) : "";
        std::string creatorStr = creator ? std::string(creator) : "";
        
        sqlite3_finalize(stmt);
        return std::make_tuple(id, fromId, toId, connectionStr, creatorStr);
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

bool LinkDatabase::updateLinkConnection(int linkId, const std::string& newConnection) {
    const char* sql = "UPDATE LINKS SET CONNECTION = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_text(stmt, 1, newConnection.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 2, linkId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool LinkDatabase::deleteLink(int linkId) {
    const char* sql = "DELETE FROM LINKS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, linkId);

    int result = sqlite3_step(stmt);
    bool success = (result == SQLITE_DONE && sqlite3_changes(db_.handle()) > 0);

    sqlite3_finalize(stmt);
    return success;
}

bool LinkDatabase::linkExists(int linkId) {
    const char* sql = "SELECT COUNT(*) FROM LINKS WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, linkId);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = (sqlite3_column_int(stmt, 0) > 0);
    }

    sqlite3_finalize(stmt);
    return exists;
}

bool LinkDatabase::linkExistsBetweenClaims(int claimIdFrom, int claimIdTo) {
    const char* sql = "SELECT COUNT(*) FROM LINKS WHERE CLAIM_ID_FROM = ? AND CLAIM_ID_TO = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, claimIdFrom);
    sqlite3_bind_int(stmt, 2, claimIdTo);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = (sqlite3_column_int(stmt, 0) > 0);
    }

    sqlite3_finalize(stmt);
    return exists;
}
