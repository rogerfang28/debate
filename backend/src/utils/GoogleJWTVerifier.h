#pragma once

#include <string>
#include <optional>
#include <map>

struct GoogleUserInfo {
    std::string sub;      // Google's unique user ID
    std::string email;
    std::string name;
    bool email_verified;
};

class GoogleJWTVerifier {
public:
    // Fetch and cache Google's JWKS (public keys). Call once at startup.
    // Returns true if keys were loaded successfully.
    static bool init();

    // Verify a Google OIDC ID token (RS256). Returns user info on success.
    // Throws std::runtime_error on verification failure.
    static GoogleUserInfo verify(const std::string& id_token);

private:
    // Decode base64url string (handles padding).
    static std::string base64url_decode(const std::string& input);

    // Parse JSON object field as string.
    static std::string json_get_string(const std::string& json, const std::string& key);

    // Parse JSON object field as string (with fallback).
    static std::optional<std::string> json_get_string_opt(const std::string& json, const std::string& key);

    // Parse JSON integer.
    static long long json_get_int(const std::string& json, const std::string& key);

    // Parse JSON boolean.
    static bool json_get_bool(const std::string& json, const std::string& key, bool default_val = false);

    // Convert base64url-encoded integer to BIGNUM.
    static void* b64url_to_bignum(const std::string& b64url);

    // OpenSSL RSA verification using base64url-encoded n and e.
    static bool rsa_verify(const std::string& signing_input,
                           const std::string& signature_b64url,
                           const std::string& n_b64url,
                           const std::string& e_b64url);
};
