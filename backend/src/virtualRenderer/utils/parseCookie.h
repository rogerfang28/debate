#pragma once
#include <string>
#include "httplib.h"

// The session cookie. One cookie, "session", holding a signed token (see
// utils/SessionToken.h): the server reads who the reader is from it only when
// the signature checks out and it has not expired. The two plain cookies it
// replaced ("user_id", "username") are ignored if a browser still sends them,
// and cleared the next time a session is set.
class parseCookie {
public:
    // The signed-in user's id, or 0 when the request carries no valid session.
    static int extractUserIdFromCookies(const httplib::Request& req);
    // The signed-in user's name, or "" when the request carries no valid session.
    static std::string extractUsernameFromCookies(const httplib::Request& req);
    // Issue a session for this user, valid for session_token::kLifetimeSeconds.
    static void setSessionCookie(const httplib::Request& req, httplib::Response& res, int user_id,
                                 const std::string& username);
    static void clearAuthCookies(const httplib::Request& req, httplib::Response& res);
};
