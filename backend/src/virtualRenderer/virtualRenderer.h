#pragma once
#include <string>
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"

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
    void handleGetRequest(const httplib::Request& req, httplib::Response& res);
    // void handlePostRequest(const httplib::Request& req, httplib::Response& res);
    ui::Page handleClientMessage(const client_message::ClientMessage& client_message, const std::string& user);

private:
    // Helper to extract user from cookies (returns "guest" if not found)
    std::string extractUserFromCookies(const httplib::Request& req);
};
