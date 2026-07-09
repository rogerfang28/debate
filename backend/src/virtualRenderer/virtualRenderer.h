#pragma once

#include <string>
#include "../database/sqlite/Database.h"
#include "../database/virtualrenderer/VRUserDatabase.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "../debateModerator/DebateModerator.h"

// Forward declarations to avoid heavy includes
namespace httplib {
    struct Request;
    struct Response;
}

/**
 * @class VirtualRenderer
 * @brief Handles HTTP requests that generate and return protobuf-encoded UI pages.
 *
 * The VirtualRenderer acts as the main bridge between HTTP endpoints and
 * the backend page generation and event handling logic. It uses:
 *  - pageGenerator (to build protobuf-serialized ui::Page messages)
 *  - EventHandler  (to process incoming debate::UIEvent messages)
 */
class VirtualRenderer {
public:
    // Constructor / Destructor
    VirtualRenderer();
    ~VirtualRenderer();

    // Utility: generate a layout (protobuf page) for a specific user
    std::string layoutGenerator(const std::string& user);

    // HTTP request handlers
     // void handleGetRequest(const httplib::Request& req, httplib::Response& res);
     // void handlePostRequest(const httplib::Request& req, httplib::Response& res);
     ui::Page handleClientMessage(const client_message::ClientMessage& client_message, const httplib::Request& req, httplib::Response& res);
     void handleAuthEvents(debate_event::DebateEvent& evt, const httplib::Request& req, httplib::Response& res);

     // Get user ID by username (public access for GET handler)
     int getUserId(const std::string& username) { return userDb.getUserId(username); }
     int createUser(const std::string& username, std::vector<uint8_t> data) { return userDb.createUser(username, data); }
     bool updateUserProtobuf(int user_id, const std::vector<uint8_t>& data) { return userDb.updateUserProtobuf(user_id, data); }

     // Load a debate page for a shared link: creates user, enters debate, returns page
     ui::Page handleDebatePageLoad(const std::string& username, int debate_id);

private:
    // Helper to extract user from cookies (returns "guest" if not found)
    int createUserIfNotExist(const std::string& username);
    std::string getUsersDatabasePath() const;
    Database usersDb;
    VRUserDatabase userDb;
    DebateModerator moderator;
};
