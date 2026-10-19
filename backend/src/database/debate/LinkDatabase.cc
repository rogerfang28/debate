#include "LinkDatabase.h"
#include <iostream>
#include "../../utils/Log.h"

#include <sstream>

namespace {
bool linkTableHasColumn(Database& db, const char* columnName) {
    const char* sql = "PRAGMA table_info(LINKS);";
    sqlite3_stmt* stmt = db.prepare(sql);
    if (!stmt) {
        return false;
    }

    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        if (name != nullptr && std::string(name) == columnName) {
            found = true;
            break;
        }
    }

    sqlite3_finalize(stmt);
    return found;
}
}

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
            CREATOR_ID INTEGER NOT NULL,
            DEBATE_ID INTEGER,
            LINK_TYPE INTEGER NOT NULL DEFAULT 0
        );
    )";
    if (!db_.execute(sql)) {
        return false;
    }

    if (!linkTableHasColumn(db_, "DEBATE_ID")) {
        if (!db_.execute("ALTER TABLE LINKS ADD COLUMN DEBATE_ID INTEGER;")) {
            return false;
        }
    }

    if (!linkTableHasColumn(db_, "LINK_TYPE")) {
        if (!db_.execute("ALTER TABLE LINKS ADD COLUMN LINK_TYPE INTEGER NOT NULL DEFAULT 0;")) {
            return false;
        }
    }

    // RELATION_ID groups rows that are ONE relation with several from-claims.
    //
    // A relation with three sources is three rows sharing a RELATION_ID, rather
    // than a new table. Two reasons: every existing reader keeps working because
    // each row is still a complete, valid single link -- it just sees three
    // supports instead of one -- and the change is one additive column instead
    // of a migration of live data into a new shape.
    //
    // Backfilled to the row's own ID, which says "each existing link is its own
    // relation". That is exactly what they were.
    if (!linkTableHasColumn(db_, "RELATION_ID")) {
        if (!db_.execute("ALTER TABLE LINKS ADD COLUMN RELATION_ID INTEGER;")) {
            return false;
        }
        if (!db_.execute("UPDATE LINKS SET RELATION_ID = ID WHERE RELATION_ID IS NULL;")) {
            return false;
        }
    }

    // TO_LINK_ID: a challenge aimed at an ARGUMENT, not a claim.
    //
    // "Your evidence is real but it does not prove that" attacks the link
    // between two claims, not either claim. The row keeps CLAIM_ID_TO -- the
    // claim the attacked argument was for -- so the many readers that only know
    // claims still see a well-formed challenge near the right place, and this
    // column names the RELATION_ID actually under attack. 0 means what every
    // row meant before: the target is CLAIM_ID_TO itself.
    if (!linkTableHasColumn(db_, "TO_LINK_ID")) {
        if (!db_.execute("ALTER TABLE LINKS ADD COLUMN TO_LINK_ID INTEGER NOT NULL DEFAULT 0;")) {
            return false;
        }
    }

    return true;
}

int LinkDatabase::addLink(int claimIdFrom, int claimIdTo, const std::string& connection, int creatorId, int debateId, int linkType, int relationId, int toLinkId) {
    const char* sql = "INSERT INTO LINKS (CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR_ID, DEBATE_ID, LINK_TYPE, RELATION_ID, TO_LINK_ID) VALUES (?, ?, ?, ?, ?, ?, ?, ?);";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return -1;
    }

    sqlite3_bind_int(stmt, 1, claimIdFrom);
    sqlite3_bind_int(stmt, 2, claimIdTo);
    sqlite3_bind_text(stmt, 3, connection.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, creatorId);
    if (debateId != -1) {
        sqlite3_bind_int(stmt, 5, debateId);
    } else {
        sqlite3_bind_null(stmt, 5);
    }
    sqlite3_bind_int(stmt, 6, linkType);
    if (relationId > 0) {
        sqlite3_bind_int(stmt, 7, relationId);
    } else {
        // Unknown until the row exists, since it defaults to the row's own id.
        sqlite3_bind_null(stmt, 7);
    }
    sqlite3_bind_int(stmt, 8, toLinkId > 0 ? toLinkId : 0);

    int result = sqlite3_step(stmt);
    int linkId = -1;

    if (result == SQLITE_DONE) {
        linkId = static_cast<int>(sqlite3_last_insert_rowid(db_.handle()));
        if (relationId <= 0) {
            // A link that is its own relation. Done as a follow-up UPDATE
            // because the id is only known after the INSERT; leaving
            // RELATION_ID NULL would make "its own relation" a second case
            // every reader has to remember to handle.
            db_.execute("UPDATE LINKS SET RELATION_ID = ID WHERE ID = " +
                        std::to_string(linkId) + ";");
        }
    } else {
        Log::error("Failed to insert link: " + std::string(sqlite3_errmsg(db_.handle())));
    }

    sqlite3_finalize(stmt);
    return linkId;
}

std::vector<std::tuple<int, int, int, std::string, int>> LinkDatabase::getLinksFromClaim(int claimIdFrom) {
    std::vector<std::tuple<int, int, int, std::string, int>> links;
    const char* sql = "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR_ID FROM LINKS WHERE CLAIM_ID_FROM = ?;";
    
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
        int creatorId = sqlite3_column_int(stmt, 4);
        
        links.push_back(std::make_tuple(id, fromId, toId, connection ? connection : "", creatorId));
    }

    sqlite3_finalize(stmt);
    return links;
}

std::vector<std::tuple<int, int, int, std::string, int>> LinkDatabase::getLinksToClaim(int claimIdTo) {
    std::vector<std::tuple<int, int, int, std::string, int>> links;
    const char* sql = "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR_ID FROM LINKS WHERE CLAIM_ID_TO = ?;";
    
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
        int creatorId = sqlite3_column_int(stmt, 4);
        
        links.push_back(std::make_tuple(id, fromId, toId, connection ? connection : "", creatorId));
    }

    sqlite3_finalize(stmt);
    return links;
}

std::vector<std::tuple<int, int, int, std::string, int>> LinkDatabase::getLinksForClaim(int claimId) {
    std::vector<std::tuple<int, int, int, std::string, int>> links;
    const char* sql = "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR_ID FROM LINKS WHERE CLAIM_ID_FROM = ? OR CLAIM_ID_TO = ?;";
    
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
        int creatorId = sqlite3_column_int(stmt, 4);
        
        links.push_back(std::make_tuple(id, fromId, toId, connection ? connection : "", creatorId));
    }

    sqlite3_finalize(stmt);
    return links;
}

std::vector<std::tuple<int, int, int, std::string, int, int, int>> LinkDatabase::getLinksForDebateAndCreators(int debateId, const std::vector<int>& creatorIds) {
    std::vector<std::tuple<int, int, int, std::string, int, int, int>> links;
    if (creatorIds.empty()) {
        return links;
    }

    std::ostringstream sql;
    sql << "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR_ID, LINK_TYPE, RELATION_ID FROM LINKS WHERE DEBATE_ID = ? AND CREATOR_ID IN (";
    for (size_t i = 0; i < creatorIds.size(); ++i) {
        if (i > 0) {
            sql << ", ";
        }
        sql << "?";
    }
    sql << ") ORDER BY ID;";

    sqlite3_stmt* stmt = db_.prepare(sql.str());
    if (!stmt) {
        return links;
    }

    sqlite3_bind_int(stmt, 1, debateId);
    for (size_t i = 0; i < creatorIds.size(); ++i) {
        sqlite3_bind_int(stmt, static_cast<int>(i + 2), creatorIds[i]);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int fromId = sqlite3_column_int(stmt, 1);
        int toId = sqlite3_column_int(stmt, 2);
        const char* connection = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        int creatorId = sqlite3_column_int(stmt, 4);
        int linkType = sqlite3_column_int(stmt, 5);
        // NULL only for a row written before the column existed and somehow
        // missed the backfill; treat it as its own relation.
        int relationId = sqlite3_column_type(stmt, 6) == SQLITE_NULL
                             ? id : sqlite3_column_int(stmt, 6);

        links.push_back(std::make_tuple(id, fromId, toId, connection ? connection : "", creatorId, linkType, relationId));
    }

    sqlite3_finalize(stmt);
    return links;
}

std::vector<std::tuple<int, int, int, std::string, int, int, int>> LinkDatabase::getLinksForDebate(int debateId) {
    std::vector<std::tuple<int, int, int, std::string, int, int, int>> links;
    const char* sql = "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR_ID, LINK_TYPE, RELATION_ID FROM LINKS WHERE DEBATE_ID = ? ORDER BY ID;";

    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return links;
    }

    sqlite3_bind_int(stmt, 1, debateId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        int fromId = sqlite3_column_int(stmt, 1);
        int toId = sqlite3_column_int(stmt, 2);
        const char* connection = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        int creatorId = sqlite3_column_int(stmt, 4);
        int linkType = sqlite3_column_int(stmt, 5);
        // NULL only for a row written before the column existed and somehow
        // missed the backfill; treat it as its own relation.
        int relationId = sqlite3_column_type(stmt, 6) == SQLITE_NULL
                             ? id : sqlite3_column_int(stmt, 6);

        links.push_back(std::make_tuple(id, fromId, toId, connection ? connection : "", creatorId, linkType, relationId));
    }

    sqlite3_finalize(stmt);
    return links;
}

std::optional<std::tuple<int, int, int, std::string, int, int, int>> LinkDatabase::getLinkById(int linkId) {
    const char* sql = "SELECT ID, CLAIM_ID_FROM, CLAIM_ID_TO, CONNECTION, CREATOR_ID, LINK_TYPE, RELATION_ID FROM LINKS WHERE ID = ?;";
    
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
        int creatorId = sqlite3_column_int(stmt, 4);
        int linkType = sqlite3_column_int(stmt, 5);
        // NULL only for a row written before the column existed and somehow
        // missed the backfill; treat it as its own relation.
        int relationId = sqlite3_column_type(stmt, 6) == SQLITE_NULL
                             ? id : sqlite3_column_int(stmt, 6);
        
        // Convert to std::string BEFORE finalizing the statement
        std::string connectionStr = connection ? std::string(connection) : "";
        
        sqlite3_finalize(stmt);
        return std::make_tuple(id, fromId, toId, connectionStr, creatorId, linkType, relationId);
    }

    sqlite3_finalize(stmt);
    return std::nullopt;
}

int LinkDatabase::getLinkDebateId(int linkId) {
    int debateId = -1;
    const char* sql = "SELECT DEBATE_ID FROM LINKS WHERE ID = ?;";

    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return debateId;
    }

    sqlite3_bind_int(stmt, 1, linkId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            debateId = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    return debateId;
}

int LinkDatabase::getLinkTargetRelation(int linkId) {
    const char* sql = "SELECT TO_LINK_ID FROM LINKS WHERE ID = ?;";
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return 0;
    }
    sqlite3_bind_int(stmt, 1, linkId);
    int target = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW && sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        target = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return target > 0 ? target : 0;
}

std::map<int, int> LinkDatabase::getRelationTargets(int debateId) {
    std::map<int, int> targets;
    const char* sql = "SELECT ID, TO_LINK_ID FROM LINKS WHERE DEBATE_ID = ? AND TO_LINK_ID > 0;";
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return targets;
    }
    sqlite3_bind_int(stmt, 1, debateId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        targets[sqlite3_column_int(stmt, 0)] = sqlite3_column_int(stmt, 1);
    }
    sqlite3_finalize(stmt);
    return targets;
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

bool LinkDatabase::updateLinkDebateId(int linkId, int debateId) {
    const char* sql = "UPDATE LINKS SET DEBATE_ID = ? WHERE ID = ?;";

    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, debateId);
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
