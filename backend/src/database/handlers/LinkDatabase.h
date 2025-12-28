#pragma once

#include <string>
#include <vector>
#include <optional>
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
    int addLink(int claimIdFrom, int claimIdTo, const std::string& connection, const std::string& creator); // return link id
    
    // Get all links from a specific claim
    std::vector<std::tuple<int, int, int, std::string, std::string>> getLinksFromClaim(int claimIdFrom); // returns (link_id, claim_id_from, claim_id_to, connection, creator)
    
    // Get all links to a specific claim
    std::vector<std::tuple<int, int, int, std::string, std::string>> getLinksToClaim(int claimIdTo); // returns (link_id, claim_id_from, claim_id_to, connection, creator)
    
    // Get all links involving a specific claim (both from and to)
    std::vector<std::tuple<int, int, int, std::string, std::string>> getLinksForClaim(int claimId); // returns (link_id, claim_id_from, claim_id_to, connection, creator)
    
    // Update the connection text for a link
    bool updateLinkConnection(int linkId, const std::string& newConnection);
    
    // Delete a link
    bool deleteLink(int linkId);
    
    // Check if a link exists
    bool linkExists(int linkId);
    
    // Check if a specific link between two claims exists
    bool linkExistsBetweenClaims(int claimIdFrom, int claimIdTo);

private:
    Database& db_;
};
