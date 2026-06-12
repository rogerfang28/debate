#pragma once

#include <string>
#include <vector>
#include <optional>
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

    // Create a new link between claims
    int addLink(int claimIdFrom, int claimIdTo, const std::string& connection, int creatorId, int debateId = -1, int linkType = 0); // return link id
    
    // Get all links from a specific claim
    std::vector<std::tuple<int, int, int, std::string, int>> getLinksFromClaim(int claimIdFrom); // returns (link_id, claim_id_from, claim_id_to, connection, creator_id)
    
    // Get all links to a specific claim
    std::vector<std::tuple<int, int, int, std::string, int>> getLinksToClaim(int claimIdTo); // returns (link_id, claim_id_from, claim_id_to, connection, creator_id)
    
    // Get all links involving a specific claim (both from and to)
    std::vector<std::tuple<int, int, int, std::string, int>> getLinksForClaim(int claimId); // returns (link_id, claim_id_from, claim_id_to, connection, creator_id)

    std::vector<std::tuple<int, int, int, std::string, int, int>> getLinksForDebate(int debateId);
    std::vector<std::tuple<int, int, int, std::string, int, int>> getLinksForDebateAndCreators(int debateId, const std::vector<int>& creatorIds);
    
    // Get a specific link by its ID
    std::optional<std::tuple<int, int, int, std::string, int, int>> getLinkById(int linkId); // returns (link_id, claim_id_from, claim_id_to, connection, creator_id, link_type)

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
