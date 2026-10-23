#include "virtualRenderer.h"
#include "layout.pb.h"
#include "client_message.pb.h"
#include "user.pb.h"
#include "user_engagement.pb.h"
#include "moderator_to_vr.pb.h"
#include "debate_event.pb.h"
#include "./ClientMessageHandler/ClientMessageParser.h"
#include "./LayoutGenerator/LayoutGenerator.h"
#include "../server/httplib.h"
#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <chrono>
#include <random>
#include <stdexcept>
#include <vector>
#include "../utils/Log.h"
#include "../utils/DemoMode.h"
#include "../utils/AnonymousUser.h"
#include "../utils/pathUtils.h"
#include "./utils/parseCookie.h"
#include "../utils/GoogleJWTVerifier.h"
#include "../utils/PasswordHasher.h"
#include "../utils/UsernamePolicy.h"
#include <ctime>

namespace {
std::string buildAutoGuestUsername() {
    using namespace std::chrono;
    const auto timestampMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    return "guest_" + std::to_string(timestampMs);
}

// A visitor name of the client's shape: "visitor-" and 8 hex digits.
std::string makeVisitorName() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<int> hex(0, 15);
    std::string name = anonymous_user::kVisitorPrefix;
    for (int i = 0; i < 8; ++i) {
        name += "0123456789abcdef"[hex(gen)];
    }
    return name;
}

// True if the string contains any whitespace (space, tab, newline, etc.).
bool hasWhitespace(const std::string& s) {
    return s.find_first_of(" \t\n\r\f\v") != std::string::npos;
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

    // All user records live in their own users.sqlite3, separate from the
    // debate data in debates.sqlite3. The DebateModerator is handed this exact
    // same connection (VirtualRenderer ctor -> DebateModerator -> DatabaseWrapper),
    // so every user ID comes from this one table and the debate DB's
    // creator_id / DEBATE_MEMBERS.USER_ID reference these IDs consistently.
    std::filesystem::path exeDir = utils::getExeDir();
    std::filesystem::path dbPath = exeDir / ".." / ".." / "users.sqlite3";
    dbPath = std::filesystem::weakly_canonical(dbPath);
    return dbPath.string();
}

// Constructor
VirtualRenderer::VirtualRenderer()
    : usersDb(getUsersDatabasePath()),
      userDb(usersDb),
      moderator(usersDb) {
    Log::info("VirtualRenderer initialized.");
}

// Destructor
VirtualRenderer::~VirtualRenderer() {
    {
        std::lock_guard<std::mutex> lock(stopMutex);
        stopping = true;
    }
    stopSignal.notify_all();
    if (maintenanceThread.joinable()) {
        maintenanceThread.join();
    }
    Log::info("VirtualRenderer destroyed."); // probably never called
}

void VirtualRenderer::startMaintenance() {
    {
        std::unique_lock<std::shared_mutex> exclusive(maintenanceMutex);
        moderator.accounts().MigrateEmailUsernames();
        moderator.accounts().SweepVisitors(static_cast<int64_t>(std::time(nullptr)));
    }
    if (maintenanceThread.joinable()) {
        return;
    }
    maintenanceThread = std::thread([this]() {
        std::unique_lock<std::mutex> wait(stopMutex);
        while (!stopSignal.wait_for(wait, kSweepInterval, [this]() { return stopping; })) {
            wait.unlock();
            {
                std::unique_lock<std::shared_mutex> exclusive(maintenanceMutex);
                moderator.accounts().SweepVisitors(static_cast<int64_t>(std::time(nullptr)));
            }
            wait.lock();
        }
    });
}

ui::Page VirtualRenderer::handleClientMessage(const client_message::ClientMessage& client_message,const httplib::Request& req, httplib::Response& res) {
    // Many requests at once, but never during a sweep (startMaintenance).
    std::shared_lock<std::shared_mutex> shared(maintenanceMutex);
    
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

        parseCookie::setSessionCookie(req, res, moderatorUserId, username);
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
    
    // Whose session this was before the request. When an anonymous visitor
    // signs in (from the sign-in modal), their place and the press they were
    // signing in to make are handed to the account they signed in as.
    const int priorUserId = user_id;

    // change cookies accordingly
    std::string loginError = handleAuthEvents(evt, req, res);

    // Renaming and closing an account change who the session is, so they
    // are carried out here, where the cookie is. A visitor has no account to
    // change; their press reaches the moderator, which does nothing with it.
    if (evt.user().is_logged_in() && !anonymous_user::IsAnonymousUsername(evt.user().username())) {
        const int uid = evt.user().user_id();
        if (evt.type() == debate_event::RENAME_ACCOUNT) {
            const std::string error = moderator.renameAccount(uid, evt.rename_account().new_username());
            const std::string current = userDb.getUsername(uid);
            if (error.empty() && !current.empty() && current != evt.user().username()) {
                // The session names the user; re-issued, or it would no longer
                // match and they would be signed out.
                parseCookie::setSessionCookie(req, res, uid, current);
                evt.mutable_user()->set_username(current);
            }
            evt.set_type(debate_event::NONE);
        } else if (evt.type() == debate_event::DELETE_ACCOUNT) {
            if (moderator.deleteAccount(uid)) {
                // Signed out: below, the request goes on as a visitor, home.
                parseCookie::clearAuthCookies(req, res);
                evt.mutable_user()->set_user_id(0);
                evt.mutable_user()->set_username("");
                evt.mutable_user()->set_is_logged_in(false);
                evt.set_type(debate_event::LOGOUT);
            } else {
                evt.set_type(debate_event::NONE);
            }
        }
    }

    // Auto-login sets response cookies, but request cookies are still empty on first load.
    if (autoLoginResolvedUserId > 0 && !autoLoginResolvedUsername.empty() && !evt.user().is_logged_in()) {
        evt.mutable_user()->set_user_id(autoLoginResolvedUserId);
        evt.mutable_user()->set_username(autoLoginResolvedUsername);
        evt.mutable_user()->set_is_logged_in(true);
    }

    // No valid session -- a first visit, a sign-out, a missing, expired or
    // unknown cookie. There is no login page: the reader is signed in as a
    // visitor, and what they asked for is not acted on as anybody (it came
    // from no one). A sign-out lands home; a failed sign-in stays a LOGIN so
    // it is answered below with the modal and its error.
    if (!evt.user().is_logged_in()) {
        const debate_event::EventType asked = evt.type();
        signInAsVisitor(evt, client_message, req, res);
        if (asked == debate_event::LOGOUT) {
            evt.set_type(debate_event::GO_HOME);
        } else if (asked != debate_event::LOGIN) {
            evt.set_type(debate_event::NONE);
        }
    }

    // A failed sign-in while a session is still alive -- a visitor's, from the
    // sign-in modal: stay on the same page with the modal open and the error
    // in it, rather than dropping them on the standalone login page.
    if (!loginError.empty() && evt.type() == debate_event::LOGIN && evt.user().is_logged_in()) {
        const std::string attemptedUsername = evt.login().username();
        evt.set_type(debate_event::NONE);
        moderator_to_vr::ModeratorToVRMessage stay = moderator.handleRequest(evt);
        return LayoutGenerator::generateLayout(stay, userDb, loginError, attemptedUsername);
    }

    const bool signedInFromVisitor =
        evt.type() == debate_event::LOGIN && evt.user().is_logged_in() &&
        priorUserId > 0 && evt.user().user_id() != priorUserId &&
        anonymous_user::IsAnonymousUsername(userDb.getUsername(priorUserId)) &&
        !anonymous_user::IsAnonymousUsername(evt.user().username());

    moderator_to_vr::ModeratorToVRMessage info;
    debate_event::DebateEvent pending;
    if (signedInFromVisitor &&
        moderator.takePendingSignIn(priorUserId, evt.user().user_id(), &pending)) {
        // The press the visitor was signing in to make, now as the signed-in
        // user. It was taken off the visitor's record already, so this is the
        // only time it runs; the moderator checks the acting user's own record,
        // which is not anonymous.
        Log::info("[VirtualRenderer] Replaying pending action type " + std::to_string(pending.type()) +
                  " as " + evt.user().username() + " (user_id=" + std::to_string(evt.user().user_id()) + ")");
        info = moderator.handleRequest(pending);
    } else {
        info = moderator.handleRequest(evt);
    }

    // After a successful Google login, transition user to HOME page -- unless
    // a visitor signed in from where they were reading, who stays there.
    if (evt.type() == debate_event::LOGIN && evt.login().has_google_id_token() &&
        !evt.login().google_id_token().empty() && evt.user().is_logged_in() && !signedInFromVisitor) {
        info.mutable_user()->mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
        Log::info("[VirtualRenderer] Google login successful — transitioning user to ACTION_HOME");
    }

    // parse user info to create layout based on it
    ui::Page page = LayoutGenerator::generateLayout(info, userDb);
    return page;
}



std::string VirtualRenderer::handleAuthEvents(debate_event::DebateEvent& evt, const httplib::Request& req, httplib::Response& res) {
    int resolvedUserId = parseCookie::extractUserIdFromCookies(req);
    std::string resolvedUsername = parseCookie::extractUsernameFromCookies(req);
    std::string loginError;

    // A signed session is one this server issued, but it can still name a
    // user that is gone (a database swapped or reset since). It counts only
    // if it names a user that exists, by the name it was issued with.
    if (resolvedUserId > 0 &&
        (resolvedUsername.empty() || userDb.getUsername(resolvedUserId) != resolvedUsername)) {
        Log::warn("[VirtualRenderer] Cookie user_id=" + std::to_string(resolvedUserId) +
                  " does not match a user; treating the request as having no session.");
        resolvedUserId = 0;
        resolvedUsername = "";
    }

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
                if (userInfo.sub.empty()) {
                    throw std::runtime_error("the token names no Google account");
                }
                AccountService& accounts = moderator.accounts();

                // A Google account is found by Google's id for it, never by
                // name: the username is the person's to change, and the email
                // is private (kept in the record's `email`, never shown).
                userId = accounts.FindByGoogleSub(userInfo.sub);
                if (userId == -1) {
                    // Made by the old sign-in, before accounts were linked:
                    // no link, no password, this email. Linked now, and given
                    // a name of its own if it is still named by the email.
                    userId = accounts.FindUnclaimedByEmail(userInfo.email);
                    if (userId != -1) {
                        updateGoogleSub(userId, userInfo.sub, userInfo.email);
                        if (userDb.getUsername(userId).find('@') != std::string::npos) {
                            const std::string error =
                                accounts.Rename(userId, accounts.UniqueNameFromEmail(userInfo.email));
                            if (!error.empty()) {
                                Log::warn("[VirtualRenderer] Could not rename linked account user_id=" +
                                          std::to_string(userId) + ": " + error);
                            }
                        }
                        Log::info("[VirtualRenderer] Linked Google account to existing user_id=" +
                                  std::to_string(userId));
                    }
                }
                if (userId == -1) {
                    // New: a username from the email's local part, made
                    // unique (bob, bob2, ...).
                    const std::string username = accounts.UniqueNameFromEmail(
                        !userInfo.email.empty() ? userInfo.email : !userInfo.name.empty() ? userInfo.name : "user");
                    Log::info("[VirtualRenderer] New Google account '" + username + "'");
                    createUserWithGoogleInfo(username, userInfo.sub, userInfo.email);
                    userId = userDb.getUserId(username);
                    if (userId == -1) {
                        throw std::runtime_error("the new account could not be created");
                    }
                }

                moderatorUserId = userId;
                finalUsername = userDb.getUsername(userId);
                Log::info("[VirtualRenderer] Google auth success: user_id=" + std::to_string(userId) + " '" +
                          finalUsername + "'");
            } catch (const std::exception& e) {
                Log::error("[VirtualRenderer] Google auth failed: " + std::string(e.what()));
                // No username field is set on a Google login event, so there is no
                // valid fallback identity here. Leave any existing cookie-based
                // session untouched instead of overwriting it with an empty user.
                authSucceeded = false;
                loginError = "Google sign-in failed. Please try again.";
            }
        } else {
            // Username-based flow.
            finalUsername = evt.login().username();
            const std::string password = evt.login().password();

            // Only a demo build signs people in without a password. Visitors
            // get their session from the visitor path, never from this form,
            // and "guest" is an ordinary name now (the guest button is gone).
            const bool passwordless = demo_mode::kDemoEnabled;

            if (passwordless) {
                moderatorUserId = moderator.createUserIfNotExist(finalUsername);
                userId = userDb.getUserId(finalUsername);
                if (userId == -1) {
                    Log::info("[VirtualRenderer] User not found, creating new user.");
                    userId = createUserIfNotExist(finalUsername);
                }
            } else if (finalUsername.empty() || password.empty()) {
                Log::warn("[VirtualRenderer] Login missing username or password.");
                authSucceeded = false;
                loginError = "Please enter both a username and a password.";
            } else if (hasWhitespace(finalUsername) || hasWhitespace(password)) {
                Log::warn("[VirtualRenderer] Username or password contains whitespace.");
                authSucceeded = false;
                loginError = "Username and password cannot contain spaces.";
            } else if (anonymous_user::IsAnonymousUsername(finalUsername)) {
                Log::warn("[VirtualRenderer] Password sign-in refused for a visitor name.");
                authSucceeded = false;
                loginError = "Names starting with \"visitor-\" are reserved. Please choose another.";
            } else if (username_policy::IsDeletedName(finalUsername)) {
                Log::warn("[VirtualRenderer] Password sign-in refused for a closed account's name.");
                authSucceeded = false;
                loginError = "Names starting with \"deleted-\" are reserved. Please choose another.";
            } else if (const std::string nameError = userDb.getUserId(finalUsername) == -1
                                                         ? moderator.accounts().CheckNewName(finalUsername)
                                                         : std::string();
                       !nameError.empty()) {
                // A new name must follow the rules (utils/UsernamePolicy.h);
                // an existing one signs in as it is.
                Log::warn("[VirtualRenderer] Registration refused for '" + finalUsername + "': " + nameError);
                authSucceeded = false;
                loginError = nameError;
            } else {
                const int existingUserId = userDb.getUserId(finalUsername);
                const bool isNewUser = (existingUserId == -1);
                moderatorUserId = moderator.createUserIfNotExist(finalUsername);

                if (isNewUser) {
                    // Auto-register: create the account and set its password.
                    userId = createUserIfNotExist(finalUsername);
                    userDb.updateUserPassword(userId, PasswordHasher::hash(password));
                    Log::info("[VirtualRenderer] Registered new user '" + finalUsername + "' with a password.");
                } else {
                    userId = existingUserId;
                    // Read the stored hash from the user's protobuf.
                    std::string storedHash;
                    auto protoData = userDb.getUserProtobufByUsername(finalUsername);
                    user::User existing;
                    if (!protoData.empty() &&
                        existing.ParseFromArray(protoData.data(), static_cast<int>(protoData.size()))) {
                        storedHash = existing.password_hash();
                    }

                    if (storedHash.empty()) {
                        // An account with no password cannot be opened with
                        // one. It used to take whatever password came first,
                        // so anyone who typed a Google user's email (or any
                        // old passwordless name) took the account over.
                        Log::warn("[VirtualRenderer] Password sign-in refused for passwordless account '" +
                                  finalUsername + "'.");
                        authSucceeded = false;
                        loginError = existing.google_sub().empty()
                            ? "This account doesn't have a password. Please choose a different username."
                            : "This account signs in with Google.";
                    } else if (!PasswordHasher::verify(password, storedHash)) {
                        Log::warn("[VirtualRenderer] Incorrect password for user '" + finalUsername + "'.");
                        authSucceeded = false;
                        loginError = "Incorrect username or password.";
                    } else {
                        Log::info("[VirtualRenderer] Password verified for user '" + finalUsername + "'.");
                    }
                }
            }
        }

        if (authSucceeded) {
            // Set cookies
            parseCookie::setSessionCookie(req, res, moderatorUserId, finalUsername);

            // Apply authenticated identity for this same request
            resolvedUserId = moderatorUserId;
            resolvedUsername = finalUsername;
        }
    }

    // add login info to the event
    evt.mutable_user()->set_user_id(resolvedUserId);
    evt.mutable_user()->set_username(resolvedUsername);
    evt.mutable_user()->set_is_logged_in(!resolvedUsername.empty() && resolvedUserId > 0);

    return loginError;
}

int VirtualRenderer::createUserIfNotExist(const std::string& username) {
    int user_id = userDb.getUserId(username);
    if (user_id == -1) {
        user::User newUser;
        newUser.set_username(username);
        newUser.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
        newUser.set_joined_at(static_cast<int64_t>(std::time(nullptr)));

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

int VirtualRenderer::createUserWithGoogleInfo(const std::string& username, const std::string& google_sub, const std::string& email) {
    int user_id = userDb.getUserId(username);
    if (user_id == -1) {
        user::User newUser;
        newUser.set_username(username);
        newUser.set_google_sub(google_sub);
        newUser.set_email(email);
        newUser.mutable_engagement()->set_current_action(user_engagement::ACTION_HOME);
        newUser.set_joined_at(static_cast<int64_t>(std::time(nullptr)));

        std::vector<uint8_t> serializedNewUser(newUser.ByteSizeLong());
        newUser.SerializeToArray(serializedNewUser.data(), serializedNewUser.size());
        user_id = userDb.createUser(username, serializedNewUser);

        newUser.set_user_id(user_id);
        serializedNewUser.resize(newUser.ByteSizeLong());
        newUser.SerializeToArray(serializedNewUser.data(), serializedNewUser.size());
        userDb.updateUserProtobuf(user_id, serializedNewUser);

        Log::info("[VirtualRenderer] Created new user with Google: " + username + " user_id=" + std::to_string(user_id));
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

    // Size the buffer to the UPDATED user, not the original blob — adding
    // google_sub/email makes it larger, and serializing into an undersized
    // buffer truncates it, corrupting the stored protobuf (causes 404 on next load).
    std::vector<uint8_t> updated(user.ByteSizeLong());
    user.SerializeToArray(updated.data(), updated.size());
    userDb.updateUserProtobuf(user_id, updated);

    Log::info("[VirtualRenderer] Updated google_sub for user_id=" + std::to_string(user_id));
}

void VirtualRenderer::signInAsVisitor(debate_event::DebateEvent& evt,
                                      const client_message::ClientMessage& message,
                                      const httplib::Request& req, httplib::Response& res) {
    // The browser keeps its visitor name in localStorage and sends it with
    // every message, so a reader who comes back (or signs out) is the same
    // visitor as before. A name not of exactly that shape is ignored.
    std::string name;
    for (const auto& comp : message.page_data().components()) {
        if (comp.id() == "visitorName" && anonymous_user::IsWellFormedVisitorName(comp.value())) {
            name = comp.value();
            break;
        }
    }
    if (name.empty()) {
        name = makeVisitorName();
    }

    const int userId = moderator.createUserIfNotExist(name);
    if (userDb.getUserId(name) == -1) {
        createUserIfNotExist(name);
    }
    // Replace any cookie this response already carries (a sign-out's
    // expiring ones): the order of repeated headers is not kept, so an
    // expiry could otherwise land after the visitor's cookie and undo it.
    res.headers.erase("Set-Cookie");
    parseCookie::setSessionCookie(req, res, userId, name);

    evt.mutable_user()->set_user_id(userId);
    evt.mutable_user()->set_username(name);
    evt.mutable_user()->set_is_logged_in(true);
    Log::info("[VirtualRenderer] No session; signed in as visitor " + name + " (user_id=" +
              std::to_string(userId) + ")");
}
