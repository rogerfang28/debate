#pragma once

#include <string>
#include <vector>
#include <optional>
#include <map>
#include <tuple>
#include "../sqlite/Database.h"

// ---------------------------------------
// Class: LinkDatabase
// ---------------------------------------
// Provides interface for managing links between claims
// using the Database wrapper class.
// ---------------------------------------
class LinkDatabase {
public:
    // Constructor takes a reference to the Database instance
    explicit LinkDatabase(Database& db);

    bool ensureTable();

    // Create a new link between claims.
    //
    // `relationId` groups rows belonging to ONE relation with several
    // from-claims. Pass 0 (the default) and the row becomes its own relation --
    // RELATION_ID is set to the new row's own ID. Pass the id returned by an
    // earlier addLink to make this row another from-claim of that relation.
    //
    // `toLinkId` > 0 makes this row a challenge to an ARGUMENT rather than to a
    // claim: it names the RELATION_ID of the link being attacked. CLAIM_ID_TO
    // still holds the claim that argument was for, so every reader that knows
    // only claims keeps working -- it sees a challenge standing next to that
    // claim -- while readers that know relations see the real target.
    int addLink(int claimIdFrom, int claimIdTo, const std::string& connection, int creatorId, int debateId = -1, int linkType = 0, int relationId = 0, int toLinkId = 0); // return link id

    // The relation a row attacks, or 0 when it targets a claim like every row
    // did before TO_LINK_ID existed.
    int getLinkTargetRelation(int linkId);

    // Every row in a debate that attacks a relation: row id -> relation id.
    // Rows that target a claim are absent, so an empty map is the common case.
    std::map<int, int> getRelationTargets(int debateId);
    
    // Get all links from a specific claim
    std::vector<std::tuple<int, int, int, std::string, int>> getLinksFromClaim(int claimIdFrom); // returns (link_id, claim_id_from, claim_id_to, connection, creator_id)
    
    // Get all links to a specific claim
    std::vector<std::tuple<int, int, int, std::string, int>> getLinksToClaim(int claimIdTo); // returns (link_id, claim_id_from, claim_id_to, connection, creator_id)
    
    // Get all links involving a specific claim (both from and to)
    std::vector<std::tuple<int, int, int, std::string, int>> getLinksForClaim(int claimId); // returns (link_id, claim_id_from, claim_id_to, connection, creator_id)

    std::vector<std::tuple<int, int, int, std::string, int, int, int>> getLinksForDebate(int debateId);
    std::vector<std::tuple<int, int, int, std::string, int, int, int>> getLinksForDebateAndCreators(int debateId, const std::vector<int>& creatorIds);
    
    // Get a specific link by its ID
    std::optional<std::tuple<int, int, int, std::string, int, int, int>> getLinkById(int linkId); // returns (link_id, claim_id_from, claim_id_to, connection, creator_id, link_type)

    int getLinkDebateId(int linkId);
    
    // Update the connection text for a link
    bool updateLinkConnection(int linkId, const std::string& newConnection);
    bool updateLinkDebateId(int linkId, int debateId);
    
    // Delete a link
    bool deleteLink(int linkId);
    
    // Check if a link exists
    bool linkExists(int linkId);
    
    // Check if a specific link between two claims exists
    bool linkExistsBetweenClaims(int claimIdFrom, int claimIdTo);

private:
    Database& db_;
};
