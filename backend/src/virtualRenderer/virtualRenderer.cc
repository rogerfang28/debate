#include "virtualRenderer.h"
#include "layout.pb.h"
#include "client_message.pb.h"
#include "user.pb.h"
#include "user_engagement.pb.h"
#include "moderator_to_vr.pb.h"
#include "debate_event.pb.h"
#include "./ClientMessageHandler/ClientMessageParser.h"
#include "./LayoutGenerator/LayoutGenerator.h"
#include "./LayoutGenerator/pages/loginPage/LoginPageGenerator.h"
#include "../server/httplib.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <vector>
#include "../utils/Log.h"
#include "../utils/DemoMode.h"
#include "../utils/pathUtils.h"
#include "./utils/parseCookie.h"
#include "../utils/GoogleJWTVerifier.h"

namespace {
std::string buildAutoGuestUsername() {
    using namespace std::chrono;
    const auto timestampMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    return "guest_" + std::to_string(timestampMs);
}
}

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
    int autoLoginResolvedUserId = 0;
    std::string autoLoginResolvedUsername;
    const bool autoLogin = demo_mode::autoLogin;
    if (autoLogin && user_id <= 0) {
        std::string username = parseCookie::extractUsernameFromCookies(req);
        if (username.empty()) {
            username = buildAutoGuestUsername();
        }

        int moderatorUserId = moderator.createUserIfNotExist(username);
        if (userDb.getUserId(username) == -1) {
            createUserIfNotExist(username);
        }

        parseCookie::setCookieUserId(req, res, moderatorUserId);
        parseCookie::setCookieUsername(req, res, username);
        user_id = moderatorUserId;
        autoLoginResolvedUserId = moderatorUserId;
        autoLoginResolvedUsername = username;

        debate_event::DebateEvent autoJoinEvent;
        autoJoinEvent.set_type(debate_event::JOIN_DEBATE);
        autoJoinEvent.mutable_join_debate()->set_debate_id(3);
        autoJoinEvent.mutable_user()->set_user_id(moderatorUserId);
        autoJoinEvent.mutable_user()->set_username(username);
        autoJoinEvent.mutable_user()->set_is_logged_in(true);
        moderator.handleRequest(autoJoinEvent);

        Log::info("[VirtualRenderer] Auto-created user for missing cookie user_id. username=" + username + " user_id=" + std::to_string(user_id));
        Log::info("[VirtualRenderer] Auto-joined user " + username + " to debate_id=1.");
    }

    debate_event::DebateEvent evt = ClientMessageParser::parseMessage(client_message, user_id);
    
    // change cookies accordingly
    handleAuthEvents(evt, req, res);

    // Auto-login sets response cookies, but request cookies are still empty on first load.
    if (autoLoginResolvedUserId > 0 && !autoLoginResolvedUsername.empty() && !evt.user().is_logged_in()) {
        evt.mutable_user()->set_user_id(autoLoginResolvedUserId);
        evt.mutable_user()->set_username(autoLoginResolvedUsername);
        evt.mutable_user()->set_is_logged_in(true);
    }
    
    moderator_to_vr::ModeratorToVRMessage info;
    info = moderator.handleRequest(evt);

    // After a successful login, transition user to HOME page
    if (evt.type() == debate_event::LOGIN && evt.login().has_google_id_token() &&
        !evt.login().google_id_token().empty() && evt.user().is_logged_in()) {
        info.mutable_user()->mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
        Log::info("[VirtualRenderer] Google login successful — transitioning user to ACTION_HOME");
    }

    // parse user info to create layout based on it
    ui::Page page = LayoutGenerator::generateLayout(info, userDb);
    return page;
}



void VirtualRenderer::handleAuthEvents(debate_event::DebateEvent& evt, const httplib::Request& req, httplib::Response& res) {
    int resolvedUserId = parseCookie::extractUserIdFromCookies(req);
    std::string resolvedUsername = parseCookie::extractUsernameFromCookies(req);

    if (evt.type() == debate_event::LOGOUT) {
        Log::info("[VirtualRenderer] Logout event detected, clearing req cookies.");
        parseCookie::clearAuthCookies(req, res);
        resolvedUserId = 0;
        resolvedUsername = "";
    }
    else if (evt.type() == debate_event::LOGIN) {
        Log::info("[VirtualRenderer] Login event detected.");

        std::string finalUsername;
        int moderatorUserId;
        int userId;
        bool authSucceeded = true;

        // Check if Google sign-in token is present
        if (evt.login().has_google_id_token() && !evt.login().google_id_token().empty()) {
            // Google OIDC flow
            Log::info("[VirtualRenderer] Google ID token received, verifying...");
            try {
                auto userInfo = GoogleJWTVerifier::verify(evt.login().google_id_token());

                // Use Google's name as fallback username, prefer email
                std::string username = userInfo.email.empty() ? userInfo.name : userInfo.email;
                if (username.empty()) {
                    username = "google_user_" + userInfo.sub.substr(0, 8);
                }

                // Look up or create user with google_sub
                moderatorUserId = moderator.createUserIfNotExist(username);
                userId = userDb.getUserId(username);
                if (userId == -1) {
                    Log::info("[VirtualRenderer] User not found, creating with google_sub=" + userInfo.sub);
                    userId = createUserWithGoogleInfo(username, userInfo.sub, userInfo.email);
                    userId = userDb.getUserId(username);
                } else {
                    // Update google_sub if not set
                    updateGoogleSub(userId, userInfo.sub, userInfo.email);
                }

                finalUsername = username;
                Log::info("[VirtualRenderer] Google auth success: " + username + " google_sub=" + userInfo.sub);
            } catch (const std::exception& e) {
                Log::error("[VirtualRenderer] Google auth failed: " + std::string(e.what()));
                // No username field is set on a Google login event, so there is no
                // valid fallback identity here. Leave any existing cookie-based
                // session untouched instead of overwriting it with an empty user.
                authSucceeded = false;
            }
        } else {
            // Legacy username-only flow
            finalUsername = evt.login().username();
            moderatorUserId = moderator.createUserIfNotExist(finalUsername);
            userId = userDb.getUserId(finalUsername);
            if (userId == -1) {
                Log::info("[VirtualRenderer] User not found, creating new user.");
                userId = createUserIfNotExist(finalUsername);
            }
        }

        if (authSucceeded) {
            // Set cookies
            parseCookie::setCookieUserId(req, res, moderatorUserId);
            parseCookie::setCookieUsername(req, res, finalUsername);

            // Apply authenticated identity for this same request
            resolvedUserId = moderatorUserId;
            resolvedUsername = finalUsername;
        }
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

ui::Page VirtualRenderer::handleDebatePageLoad(const std::string& username, int debate_id) {
    Log::info("[VirtualRenderer] handleDebatePageLoad: username=" + username + " debate_id=" + std::to_string(debate_id));

    // 1. Create user in BOTH databases so MoveUserHandler::EnterDebate can find it
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
        Log::info("[VirtualRenderer] Created user in VR DB: " + username + " (id=" + std::to_string(user_id) + ")");
    } else {
        Log::info("[VirtualRenderer] User already in VR DB: " + username + " (id=" + std::to_string(user_id) + ")");
    }

    // Also ensure user exists in the main DebateModerator DB (dbWrapper.users)
    int moderatorUserId = moderator.createUserIfNotExist(username);
    if (moderatorUserId != user_id) {
        Log::info("[VirtualRenderer] User ID mismatch: VR DB=" + std::to_string(user_id) + " vs Moderator DB=" + std::to_string(moderatorUserId));
    }
    Log::info("[VirtualRenderer] Moderator DB user_id: " + std::to_string(moderatorUserId));

    // 2. Send ENTER_DEBATE event to load the debate
    debate_event::DebateEvent evt;
    evt.set_type(debate_event::ENTER_DEBATE);
    evt.mutable_enter_debate()->set_debate_id(debate_id);
    evt.mutable_user()->set_user_id(moderatorUserId);
    evt.mutable_user()->set_username(username);
    evt.mutable_user()->set_is_logged_in(true);

    moderator_to_vr::ModeratorToVRMessage info = moderator.handleRequest(evt);

    // 3. Generate layout page
    return LayoutGenerator::generateLayout(info, userDb);
}

int VirtualRenderer::createUserWithGoogleInfo(const std::string& username, const std::string& google_sub, const std::string& email) {
    int user_id = userDb.getUserId(username);
    if (user_id == -1) {
        user::User newUser;
        newUser.set_username(username);
        newUser.set_google_sub(google_sub);
        newUser.set_email(email);
        newUser.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);

        std::vector<uint8_t> serializedNewUser(newUser.ByteSizeLong());
        newUser.SerializeToArray(serializedNewUser.data(), serializedNewUser.size());
        user_id = userDb.createUser(username, serializedNewUser);

        newUser.set_user_id(user_id);
        serializedNewUser.resize(newUser.ByteSizeLong());
        newUser.SerializeToArray(serializedNewUser.data(), serializedNewUser.size());
        userDb.updateUserProtobuf(user_id, serializedNewUser);

        Log::info("[VirtualRenderer] Created new user with Google: " + username + " user_id=" + std::to_string(user_id) + " google_sub=" + google_sub);
    }
    return user_id;
}

void VirtualRenderer::updateGoogleSub(int user_id, const std::string& google_sub, const std::string& email) {
    auto protoData = userDb.getUserProtobuf(user_id);
    if (protoData.empty()) return;

    user::User user;
    if (!user.ParseFromArray(protoData.data(), (int)protoData.size())) return;

    if (!user.google_sub().empty()) return; // already set

    user.set_google_sub(google_sub);
    if (!email.empty()) user.set_email(email);

    std::vector<uint8_t> updated(protoData.size());
    user.SerializeToArray(updated.data(), updated.size());
    userDb.updateUserProtobuf(user_id, updated);

    Log::info("[VirtualRenderer] Updated google_sub for user_id=" + std::to_string(user_id));
}
