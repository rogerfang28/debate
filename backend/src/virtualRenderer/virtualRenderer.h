#pragma once
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <thread>
#include "../database/sqlite/Database.h"
#include "../database/virtualrenderer/VRUserDatabase.h"
#include "layout.pb.h"
#include "client_message.pb.h"
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
    // Resolves the request's identity (cookies, Google, or username+password)
    // and mutates evt.user() accordingly. Returns a user-facing error message
    // if a login attempt failed (e.g. wrong password); empty string otherwise.
    std::string handleAuthEvents(debate_event::DebateEvent& evt, const httplib::Request& req, httplib::Response& res);

    // The account jobs the server runs on its own: the email-name migration
    // and a visitor sweep now, then a sweep every kSweepInterval on a
    // background thread. Requests wait while a job runs (and a job waits
    // for the requests in flight), so nobody is deleted mid-request.
    void startMaintenance();
    static constexpr std::chrono::hours kSweepInterval{6};
private:
    // Helper to extract user from cookies (returns "guest" if not found)
     int createUserIfNotExist(const std::string& username);
     int createUserWithGoogleInfo(const std::string& username, const std::string& google_sub, const std::string& email);
     void updateGoogleSub(int user_id, const std::string& google_sub, const std::string& email);
     std::string getUsersDatabasePath() const;
     // A request with no valid session: signs it in as a visitor (the
     // browser's own visitorName when it sent a well-formed one, else a new
     // one), sets the session cookies, and makes evt act as that visitor.
     void signInAsVisitor(debate_event::DebateEvent& evt, const client_message::ClientMessage& message,
                          const httplib::Request& req, httplib::Response& res);
    Database usersDb;
    VRUserDatabase userDb;
    DebateModerator moderator;

    // Shared by every request, exclusive for a maintenance job.
    std::shared_mutex maintenanceMutex;
    std::thread maintenanceThread;
    std::mutex stopMutex;
    std::condition_variable stopSignal;
    bool stopping = false;
};
