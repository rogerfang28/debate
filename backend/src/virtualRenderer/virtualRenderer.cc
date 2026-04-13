#include "virtualRenderer.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "../../../src/gen/cpp/user.pb.h"
#include "../../../src/gen/cpp/user_engagement.pb.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "./ClientMessageHandler/ClientMessageParser.h"
#include "./LayoutGenerator/LayoutGenerator.h"
#include "./LayoutGenerator/pages/loginPage/LoginPageGenerator.h"
// #include "../debate/main/EventHandler.h"
#include "../server/httplib.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <vector>
#include "../utils/Log.h"
#include "../utils/pathUtils.h"
#include "./utils/parseCookie.h"

std::string VirtualRenderer::getUsersDatabasePath() const {
    if (const char* envPath = std::getenv("USER_DB_PATH"); envPath && envPath[0] != '\0') {
        std::filesystem::path configured = std::filesystem::path(envPath);
        if (configured.is_relative()) {
            configured = std::filesystem::current_path() / configured;
        }
        return configured.lexically_normal().string();
    }

    std::filesystem::path exeDir = utils::getExeDir();
    std::filesystem::path dbPath = exeDir / ".." / ".." / "users.sqlite3";
    dbPath = std::filesystem::weakly_canonical(dbPath);
    return dbPath.string();
}

// Constructor
VirtualRenderer::VirtualRenderer()
    : usersDb(getUsersDatabasePath()),
      userDb(usersDb) {
    Log::info("VirtualRenderer initialized.");
}

// Destructor
VirtualRenderer::~VirtualRenderer() {
    Log::info("VirtualRenderer destroyed."); // probably never called
}

ui::Page VirtualRenderer::handleClientMessage(const client_message::ClientMessage& client_message,const httplib::Request& req, httplib::Response& res) {
    
    // translate client_message into debate event
    int user_id = parseCookie::extractUserIdFromCookies(req);
    debate_event::DebateEvent evt = ClientMessageParser::parseMessage(client_message, user_id);
    // BackendCommunicator backend("localhost", 3000);
    // ! no server call for now, backend and virtual renderer are on the same backend
    
    // change cookies accordingly
    handleAuthEvents(evt, req, res);
    
    moderator_to_vr::ModeratorToVRMessage info;
    info = moderator.handleRequest(evt);
    // backend.sendEvent(evt, info);

    // parse user info to create layout based on it
    ui::Page page = LayoutGenerator::generateLayout(info, userDb);
    return page;
}



void VirtualRenderer::handleAuthEvents(debate_event::DebateEvent& evt, const httplib::Request& req, httplib::Response& res) {
    int resolvedUserId = parseCookie::extractUserIdFromCookies(req);
    std::string resolvedUsername = parseCookie::extractUsernameFromCookies(req);

    if (evt.type() == debate_event::LOGOUT) {
        Log::info("[VirtualRenderer] Logout event detected, clearing req cookies.");
        parseCookie::clearAuthCookies(res);
        resolvedUserId = 0;
        resolvedUsername = "";
    }
    else if (evt.type() == debate_event::LOGIN) {
        Log::info("[VirtualRenderer] Login event detected, login the user with cookies.");
        int moderatorUserId = moderator.createUserIfNotExist(evt.login().username());
        int userId = userDb.getUserId(evt.login().username());
        if (userId == -1) {
            Log::info("[VirtualRenderer] User not found in virtual renderer DB, creating new user.");
            createUserIfNotExist(evt.login().username());
            userId = userDb.getUserId(evt.login().username());
        }
        // this ensures a user exists
        parseCookie::setCookieUserId(res, moderatorUserId);
        parseCookie::setCookieUsername(res, evt.login().username());

        // Apply authenticated identity for this same request, not only future requests.
        resolvedUserId = moderatorUserId;
        resolvedUsername = evt.login().username();
    }

    // add login info to the event
    evt.mutable_user()->set_user_id(resolvedUserId);
    evt.mutable_user()->set_username(resolvedUsername);
    evt.mutable_user()->set_is_logged_in(!resolvedUsername.empty() && resolvedUserId > 0);
}

int VirtualRenderer::createUserIfNotExist(const std::string& username) {
    int user_id = userDb.getUserId(username);
    if (user_id == -1) {
        user::User newUser;
        newUser.set_username(username);
        newUser.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);

        std::vector<uint8_t> serializedNewUser(newUser.ByteSizeLong());
        newUser.SerializeToArray(serializedNewUser.data(), serializedNewUser.size());
        user_id = userDb.createUser(username, serializedNewUser);

        newUser.set_user_id(user_id);
        serializedNewUser.resize(newUser.ByteSizeLong());
        newUser.SerializeToArray(serializedNewUser.data(), serializedNewUser.size());
        userDb.updateUserProtobuf(user_id, serializedNewUser);

        Log::info("[VirtualRenderer] Created new user: " + username + " with user_id: " + std::to_string(user_id));
    }
    return user_id;
}