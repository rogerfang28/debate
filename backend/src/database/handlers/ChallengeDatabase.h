#pragma once

#include <string>
#include <vector>
#include <map>
#include <optional>
#include "../sqlite/Database.h"

// ---------------------------------------
// Class: ChallengeDatabase
// ---------------------------------------
// Provides interface for managing challenges
// using the Database wrapper class.
// ---------------------------------------
class ChallengeDatabase {
public:
    // Constructor takes a reference to the Database instance
    explicit ChallengeDatabase(Database& db);

    bool ensureTable();

    int addChallenge(const std::vector<uint8_t>& protobufData, int creatorId, int challengedClaimId); // return challenge id
    std::vector<uint8_t> getChallengeProtobuf(int challengeId);
    int getChallengeCreatorId(int challengeId);
    int getChallengedClaimId(int challengeId);
    std::vector<int> getChallengesAgainstClaim(int claimId); // return vector of challenge ids

    bool updateChallengeProtobuf(int challengeId, const std::vector<uint8_t>& protobufData);
    bool updateChallengeCreatorId(int challengeId, int creatorId);
    bool updateChallengedClaimId(int challengeId, int challengedClaimId);
    bool deleteChallenge(int challengeId);
    bool challengeExists(int challengeId);

private:
    Database& db_;
};
