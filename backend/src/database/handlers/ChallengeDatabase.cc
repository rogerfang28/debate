#include "ChallengeDatabase.h"
#include <iostream>
#include "../../utils/Log.h"

ChallengeDatabase::ChallengeDatabase(Database& db) : db_(db) {
    ensureTable();
}

bool ChallengeDatabase::ensureTable() {
    const char* sql = R"(
        CREATE TABLE IF NOT EXISTS CHALLENGES (
            ID INTEGER PRIMARY KEY AUTOINCREMENT,
            CHALLENGE_DATA BLOB,
            CREATOR_ID INTEGER,
            CHALLENGED_CLAIM_ID INTEGER
        );
    )";
    return db_.execute(sql);
}

int ChallengeDatabase::addChallenge(const std::vector<uint8_t>& protobufData, int creatorId, int challengedClaimId) {
    const char* sql = "INSERT INTO CHALLENGES (CHALLENGE_DATA, CREATOR_ID, CHALLENGED_CLAIM_ID) VALUES (?, ?, ?);";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return -1;
    }

    if (!protobufData.empty()) {
        sqlite3_bind_blob(stmt, 1, protobufData.data(), protobufData.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 1);
    }

    sqlite3_bind_int(stmt, 2, creatorId);
    sqlite3_bind_int(stmt, 3, challengedClaimId);

    int result = sqlite3_step(stmt);
    int challengeId = -1;
    
    if (result == SQLITE_DONE) {
        challengeId = static_cast<int>(sqlite3_last_insert_rowid(db_.handle()));
    } else {
        std::cerr << "Failed to insert challenge: " << sqlite3_errmsg(db_.handle()) << std::endl;
    }

    sqlite3_finalize(stmt);
    return challengeId;
}

std::vector<uint8_t> ChallengeDatabase::getChallengeProtobuf(int challengeId) {
    std::vector<uint8_t> protobufData;
    const char* sql = "SELECT CHALLENGE_DATA FROM CHALLENGES WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return protobufData;
    }

    sqlite3_bind_int(stmt, 1, challengeId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const void* blob = sqlite3_column_blob(stmt, 0);
        int blobSize = sqlite3_column_bytes(stmt, 0);
        
        if (blob && blobSize > 0) {
            const uint8_t* data = static_cast<const uint8_t*>(blob);
            protobufData.assign(data, data + blobSize);
        }
    }

    sqlite3_finalize(stmt);
    return protobufData;
}

int ChallengeDatabase::getChallengeCreatorId(int challengeId) {
    int creatorId = -1;
    const char* sql = "SELECT CREATOR_ID FROM CHALLENGES WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return creatorId;
    }

    sqlite3_bind_int(stmt, 1, challengeId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            creatorId = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    return creatorId;
}

int ChallengeDatabase::getChallengedClaimId(int challengeId) {
    int challengedClaimId = -1;
    const char* sql = "SELECT CHALLENGED_CLAIM_ID FROM CHALLENGES WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return challengedClaimId;
    }

    sqlite3_bind_int(stmt, 1, challengeId);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        if (sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
            challengedClaimId = sqlite3_column_int(stmt, 0);
        }
    }

    sqlite3_finalize(stmt);
    return challengedClaimId;
}

std::vector<int> ChallengeDatabase::getChallengesAgainstClaim(int claimId) {
    std::vector<int> challengeIds;
    const char* sql = "SELECT ID FROM CHALLENGES WHERE CHALLENGED_CLAIM_ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return challengeIds;
    }

    sqlite3_bind_int(stmt, 1, claimId);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        challengeIds.push_back(sqlite3_column_int(stmt, 0));
    }

    sqlite3_finalize(stmt);
    return challengeIds;
}

bool ChallengeDatabase::updateChallengeProtobuf(int challengeId, const std::vector<uint8_t>& protobufData) {
    const char* sql = "UPDATE CHALLENGES SET CHALLENGE_DATA = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    if (!protobufData.empty()) {
        sqlite3_bind_blob(stmt, 1, protobufData.data(), protobufData.size(), SQLITE_TRANSIENT);
    } else {
        sqlite3_bind_null(stmt, 1);
    }
    sqlite3_bind_int(stmt, 2, challengeId);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return result == SQLITE_DONE;
}

bool ChallengeDatabase::updateChallengeCreatorId(int challengeId, int creatorId) {
    const char* sql = "UPDATE CHALLENGES SET CREATOR_ID = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, creatorId);
    sqlite3_bind_int(stmt, 2, challengeId);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return result == SQLITE_DONE;
}

bool ChallengeDatabase::updateChallengedClaimId(int challengeId, int challengedClaimId) {
    const char* sql = "UPDATE CHALLENGES SET CHALLENGED_CLAIM_ID = ? WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, challengedClaimId);
    sqlite3_bind_int(stmt, 2, challengeId);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return result == SQLITE_DONE;
}

bool ChallengeDatabase::deleteChallenge(int challengeId) {
    const char* sql = "DELETE FROM CHALLENGES WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, challengeId);

    int result = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return result == SQLITE_DONE;
}

bool ChallengeDatabase::challengeExists(int challengeId) {
    const char* sql = "SELECT COUNT(*) FROM CHALLENGES WHERE ID = ?;";
    
    sqlite3_stmt* stmt = db_.prepare(sql);
    if (!stmt) {
        return false;
    }

    sqlite3_bind_int(stmt, 1, challengeId);

    bool exists = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt, 0) > 0;
    }

    sqlite3_finalize(stmt);
    return exists;
}
