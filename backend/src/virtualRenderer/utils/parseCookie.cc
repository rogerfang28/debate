#include "parseCookie.h"
#include "../../utils/Log.h"
#include "../../utils/SessionToken.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <optional>
#include <string>

namespace {
bool envFlagEnabled(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return false;
    }

    std::string normalized(value);
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });

    return normalized == "1" || normalized == "true" || normalized == "yes" || normalized == "on";
}

bool isLocalOrigin(const std::string& value) {
    return value.find("localhost") != std::string::npos || value.find("127.0.0.1") != std::string::npos;
}

bool isHttpsRequest(const httplib::Request& req) {
    const std::string origin = req.get_header_value("Origin");
    const std::string host = req.get_header_value("Host");
    return origin.find("https://") != std::string::npos || host.find(":443") != std::string::npos;
}

std::string cookieAttributesFromRequest(const httplib::Request& req) {
    std::string sameSite = "Lax";
    bool secure = false;

    if (const char* mode = std::getenv("DEBATE_COOKIE_MODE"); mode != nullptr && mode[0] != '\0') {
        std::string normalized(mode);
        std::transform(normalized.begin(), normalized.end(), normalized.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });

        if (normalized == "hosted" || normalized == "production" || normalized == "https") {
            sameSite = "None";
            secure = true;
        }
    }
    else {
        const std::string origin = req.get_header_value("Origin");
        const std::string host = req.get_header_value("Host");
        const bool hostedRequest = (!origin.empty() && !isLocalOrigin(origin)) || (!host.empty() && !isLocalOrigin(host));

        if (hostedRequest) {
            // Only use Secure + SameSite=None for HTTPS
            if (isHttpsRequest(req)) {
                sameSite = "None";
                secure = true;
            } else {
                // HTTP hosted request — use Lax so cookies work over plain HTTP
                sameSite = "Lax";
                secure = false;
            }
        }
    }

    if (const char* sameSiteEnv = std::getenv("DEBATE_COOKIE_SAMESITE"); sameSiteEnv != nullptr && sameSiteEnv[0] != '\0') {
        sameSite = sameSiteEnv;
    }

    if (std::getenv("DEBATE_COOKIE_SECURE") != nullptr) {
        secure = envFlagEnabled("DEBATE_COOKIE_SECURE");
    }

    std::string attributes = "; Path=/; HttpOnly";
    if (!sameSite.empty()) {
        attributes += "; SameSite=" + sameSite;
    }
    if (secure) {
        attributes += "; Secure";
    }

    if (sameSite == "None" && !secure) {
        Log::info("[Auth] Cookie policy uses SameSite=None without Secure; browsers may reject it on modern builds.");
    }

    return attributes;
}
}

namespace {

constexpr const char* kSessionCookie = "session";
// Before sessions were signed: plain values anyone could set. Never read;
// expired whenever a session is set or cleared.
constexpr const char* kLegacyCookies[] = {"user_id", "username"};

// The value of the cookie with exactly this name, if the request has one.
// Matching the whole name matters: a substring search would read "user_id"
// out of "xuser_id", or a cookie's name out of another cookie's value.
std::string cookieValue(const httplib::Request& req, const std::string& name) {
    const std::string header = req.get_header_value("Cookie");
    size_t start = 0;
    while (start < header.size()) {
        size_t end = header.find(';', start);
        if (end == std::string::npos) end = header.size();
        std::string pair = header.substr(start, end - start);
        const size_t first = pair.find_first_not_of(' ');
        pair = first == std::string::npos ? "" : pair.substr(first);
        const size_t eq = pair.find('=');
        if (eq != std::string::npos && pair.substr(0, eq) == name) {
            return pair.substr(eq + 1);
        }
        start = end + 1;
    }
    return "";
}

std::optional<session_token::Session> sessionFrom(const httplib::Request& req) {
    const std::string token = cookieValue(req, kSessionCookie);
    if (token.empty()) return std::nullopt;
    auto session = session_token::Verify(token, session_token::NowSeconds(), session_token::ServerSecret());
    if (!session) {
        Log::warn("[Auth] Session cookie is invalid or expired; treating the request as having no session.");
    }
    return session;
}

void expire(const httplib::Request& req, httplib::Response& res, const std::string& name) {
    res.set_header("Set-Cookie", name + "=; Expires=Thu, 01 Jan 1970 00:00:00 GMT" +
                                     cookieAttributesFromRequest(req));
}

}  // namespace

int parseCookie::extractUserIdFromCookies(const httplib::Request& req) {
    const auto session = sessionFrom(req);
    return session ? session->userId : 0;
}

std::string parseCookie::extractUsernameFromCookies(const httplib::Request& req) {
    const auto session = sessionFrom(req);
    return session ? session->username : "";
}

void parseCookie::setSessionCookie(const httplib::Request& req, httplib::Response& res, int user_id,
                                   const std::string& username) {
    const std::string token = session_token::Sign(
        user_id, username, session_token::NowSeconds() + session_token::kLifetimeSeconds,
        session_token::ServerSecret());
    // No Expires/Max-Age: the cookie goes when the browser closes, as the
    // privacy page says. The 30 days are a ceiling inside the signed value.
    res.set_header("Set-Cookie", std::string(kSessionCookie) + "=" + token + cookieAttributesFromRequest(req));
    for (const char* legacy : kLegacyCookies) {
        if (!cookieValue(req, legacy).empty()) expire(req, res, legacy);
    }
}

void parseCookie::clearAuthCookies(const httplib::Request& req, httplib::Response& res) {
    expire(req, res, kSessionCookie);
    for (const char* legacy : kLegacyCookies) {
        expire(req, res, legacy);
    }
}
