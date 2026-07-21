#pragma once
#include <string>
#include <vector>
#include <mutex>
#include "httplib.h"

// ---------------------------------------
// Class: HMAC
// ---------------------------------------
// HMAC-SHA256 implementation (RFC 2104)
// Uses a shared secret key configured via DEBATE_COOKIE_SECRET env var.
// This prevents cookie value forgery (IDOR/auth bypass).
// ---------------------------------------
class HMAC {
public:
    // Returns the HMAC-SHA256 signature of data with the configured key.
    static std::string sign(const std::string& data, const std::string& key);

    // Verifies that `signature` is a valid HMAC-SHA256 of `data` using `key`.
    static bool verify(const std::string& data, const std::string& sig, const std::string& key);

    // Reads the shared secret from env var DEBATE_COOKIE_SECRET.
    // Defaults to a hardcoded string if not set (WARNING — set it in production!).
    static std::string getSecretKey();
};

// ---------------------------------------
// Class: parseCookie
// ---------------------------------------
// Provides interface for managing authentication cookies.
// All cookie values are now HMAC-signed to prevent tampering.
// ---------------------------------------
class parseCookie {
public:
    // Extract user_id from signed cookies.
    // Returns 0 (guest) if cookie is missing, invalid, expired, or tampered with.
    static int extractUserIdFromCookies(const httplib::Request& req);
    static std::string extractUsernameFromCookies(const httplib::Request& req);

    // Set authenticated cookies with HMAC signature and Secure/HttpOnly flags.
    static void setCookieUsername(httplib::Response& res, std::string username);
    static void setCookieUserId(httplib::Response& res, int user_id);

    // Sign + base64-encode user data for cookie storage.
    static std::string signUserData(int user_id, const std::string& username);

    // Verify signature and decode signed cookie data.
    // Returns empty string if signature is invalid or decoding fails.
    static std::string verifyCookieData(const std::string& signed_value);

    // Clear auth cookies (expiration).
    static void clearAuthCookies(httplib::Response& res);

private:
    // Decode a base64-encoded string.
    static std::vector<unsigned char> base64Decode(const std::string& input);

    // Encode data to base64.
    static std::string base64Encode(const std::vector<unsigned char>& input);
};