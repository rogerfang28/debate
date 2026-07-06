#include "GoogleJWTVerifier.h"
#include "../utils/Log.h"
#include "../server/httplib.h"
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <stdexcept>
#include <cstring>

namespace {
// Google's JWKS endpoint
const std::string JWKS_URL = "https://www.googleapis.com/oauth2/v3/certs";

// Cached JWKS keys: kid -> {n, e}
// Cached at startup, refreshed on unknown kid.
std::map<std::string, std::map<std::string, std::string>> g_jwks_cache;
std::chrono::system_clock::time_point g_jwks_cache_time;
const int JWKS_CACHE_SECONDS = 300; // 5 minutes

// Base64url decode table
const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

// Find char in base64url table, -1 if not found
int b64_find(char c) {
    for (int i = 0; i < 256; i++) {
        if (b64_table[i] == c) return i;
    }
    // base64url uses - and _ instead of + and /
    if (c == '-') return 62;
    if (c == '_') return 63;
    return -1;
}

std::string url_to_standard(const std::string& b64url) {
    std::string result = b64url;
    for (auto& c : result) {
        if (c == '-') c = '+';
        if (c == '_') c = '/';
    }
    // Add padding
    while (result.size() % 4 != 0) result += '=';
    return result;
}

std::string hex_from_bn(void* bn) {
    // Not used directly; we work with raw bytes
    return "";
}

} // anonymous namespace

bool GoogleJWTVerifier::init() {
    try {
        std::string jwks = fetch_jwks();
        g_jwks_cache = parse_jwks(jwks);
        g_jwks_cache_time = std::chrono::system_clock::now();
        Log::info("[GoogleAuth] Loaded " + std::to_string(g_jwks_cache.size()) + " JWKS keys");
        return !g_jwks_cache.empty();
    } catch (const std::exception& e) {
        Log::error("[GoogleAuth] Failed to init JWKS: " + std::string(e.what()));
        return false;
    }
}

GoogleJWTVerifier::GoogleUserInfo GoogleJWTVerifier::verify(const std::string& id_token) {
    if (id_token.empty() || id_token.size() < 10) {
        throw std::runtime_error("Invalid token: too short");
    }

    // Split JWT into parts
    std::vector<std::string> parts;
    std::istringstream iss(id_token);
    std::string part;
    while (std::getline(iss, part, '.')) {
        parts.push_back(part);
    }
    if (parts.size() != 3) {
        throw std::runtime_error("Invalid token: expected 3 parts");
    }

    // Decode header
    std::string header_json = base64url_decode(parts[0]);
    std::string alg = json_get_string(header_json, "alg");
    std::string kid = json_get_string(header_json, "kid");

    if (alg != "RS256") {
        throw std::runtime_error("Unsupported algorithm: " + alg);
    }

    // Check if we need to refresh JWKS
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - g_jwks_cache_time).count();
    if (elapsed > JWKS_CACHE_SECONDS || kid.empty() || g_jwks_cache.find(kid) == g_jwks_cache.end()) {
        Log::info("[GoogleAuth] Refreshing JWKS (elapsed=" + std::to_string(elapsed) + "s, kid=" + kid + ")");
        std::string jwks = fetch_jwks();
        g_jwks_cache = parse_jwks(jwks);
        g_jwks_cache_time = std::chrono::system_clock::now();

        if (g_jwks_cache.find(kid) == g_jwks_cache.end()) {
            throw std::runtime_error("Unknown kid: " + kid);
        }
    }

    auto& key = g_jwks_cache[kid];
    const std::string& n = key["n"];
    const std::string& e = key["e"];

    // Verify signature
    std::string signature_b64url = parts[2];
    std::string signing_input = parts[0] + "." + parts[1];

    if (!verify_signature(signing_input, signature_b64url, n, e)) {
        throw std::runtime_error("Signature verification failed");
    }

    // Decode and validate payload
    std::string payload_json = base64url_decode(parts[1]);

    // Validate iss
    std::string iss = json_get_string(payload_json, "iss");
    if (iss != "accounts.google.com" && iss != "https://accounts.google.com") {
        throw std::runtime_error("Invalid iss: " + iss);
    }

    // Validate exp
    long long exp = json_get_int(payload_json, "exp");
    auto now_epoch = std::chrono::system_clock::to_time_t(now);
    if (exp <= (long long)now_epoch + 60) { // Allow 60s clock skew
        throw std::runtime_error("Token expired");
    }

    // Validate aud == GOOGLE_CLIENT_ID
    std::string aud = json_get_string(payload_json, "aud");
    const char* client_id = std::getenv("GOOGLE_CLIENT_ID");
    if (client_id && !aud.empty() && aud != client_id) {
        throw std::runtime_error("Invalid aud: expected " + std::string(client_id) + ", got " + aud);
    }

    // Extract user info
    GoogleUserInfo info;
    info.sub = json_get_string(payload_json, "sub");
    info.email = json_get_string(payload_json, "email");
    info.name = json_get_string(payload_json, "name");
    info.email_verified = json_get_bool(payload_json, "email_verified", false);

    if (info.sub.empty()) {
        throw std::runtime_error("Missing sub in token payload");
    }

    Log::info("[GoogleAuth] Verified token for sub=" + info.sub + " email=" + info.email);
    return info;
}

std::string GoogleJWTVerifier::base64url_decode(const std::string& input) {
    std::string standard = url_to_standard(input);
    std::string result;
    int in_pos = 0;
    int in_len = (int)standard.size();

    while (in_pos < in_len) {
        int a = b64_find(standard[in_pos++]);
        int b = b64_find(standard[in_pos++]);
        int c = b64_find(standard[in_pos++]);
        int d = b64_find(standard[in_pos++]);

        int triplet = (a << 18) | (b << 12) | (c << 6) | d;

        if (a >= 0) result += (char)((triplet >> 16) & 0xFF);
        if (b >= 0 && in_pos <= in_len - 1) result += (char)((triplet >> 8) & 0xFF);
        if (c >= 0 && in_pos <= in_len - 2) result += (char)(triplet & 0xFF);
    }

    return result;
}

std::string GoogleJWTVerifier::json_get_string(const std::string& json, const std::string& key) {
    auto opt = json_get_string_opt(json, key);
    return opt.value_or("");
}

std::optional<std::string> GoogleJWTVerifier::json_get_string_opt(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return std::nullopt;

    pos = json.find(':', pos + search_key.size());
    if (pos == std::string::npos) return std::nullopt;

    // Skip whitespace
    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
        pos++;
    }

    if (pos >= json.size() || json[pos] != '"') return std::nullopt;
    pos++; // skip opening quote

    size_t end = pos;
    while (end < json.size() && json[end] != '"') {
        if (json[end] == '\\' && end + 1 < json.size()) end++;
        end++;
    }

    return json.substr(pos, end - pos);
}

long long GoogleJWTVerifier::json_get_int(const std::string& json, const std::string& key) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return 0;

    pos = json.find(':', pos + search_key.size());
    if (pos == std::string::npos) return 0;

    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    size_t end = pos;
    while (end < json.size() && (json[end] >= '0' && json[end] <= '9')) end++;

    std::string num_str = json.substr(pos, end - pos);
    try {
        return std::stoll(num_str);
    } catch (...) {
        return 0;
    }
}

bool GoogleJWTVerifier::json_get_bool(const std::string& json, const std::string& key, bool default_val) {
    std::string search_key = "\"" + key + "\"";
    size_t pos = json.find(search_key);
    if (pos == std::string::npos) return default_val;

    pos = json.find(':', pos + search_key.size());
    if (pos == std::string::npos) return default_val;

    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t')) pos++;

    if (pos + 4 <= json.size() && json.substr(pos, 4) == "true") return true;
    if (pos + 5 <= json.size() && json.substr(pos, 5) == "false") return false;
    return default_val;
}

std::string GoogleJWTVerifier::fetch_jwks() {
    httplib::Client cli("www.googleapis.com", 443, "");

    auto res = cli.Get("/oauth2/v3/certs");
    if (!res || res->status != 200) {
        throw std::runtime_error("Failed to fetch JWKS: " + (res ? std::to_string(res->status) : "no response"));
    }

    // Update cache time from Cache-Control header
    auto cc = res->get_header_value("Cache-Control");
    size_t max_age = cc.find("max-age=");
    if (max_age != std::string::npos) {
        std::string age_str = cc.substr(max_age + 8);
        size_t comma = age_str.find(',');
        if (comma != std::string::npos) age_str = age_str.substr(0, comma);
        try {
            int age = std::stoi(age_str);
            if (age > 0 && age < 86400) {
                g_jwks_cache_time = std::chrono::system_clock::now() - std::chrono::seconds(age);
            }
        } catch (...) {}
    }

    return res->body;
}

std::map<std::string, std::map<std::string, std::string>> GoogleJWTVerifier::parse_jwks(const std::string& jwks_json) {
    std::map<std::string, std::map<std::string, std::string>> keys;

    // Parse the "keys" array from JSON
    size_t keys_pos = jwks_json.find("\"keys\"");
    if (keys_pos == std::string::npos) {
        throw std::runtime_error("No keys in JWKS response");
    }

    size_t arr_start = jwks_json.find('[', keys_pos);
    if (arr_start == std::string::npos) {
        throw std::runtime_error("Invalid JWKS format");
    }

    // Find each key object - naive approach: split by "kid" occurrences
    size_t pos = arr_start;
    while (pos < jwks_json.size()) {
        size_t kid_pos = jwks_json.find("\"kid\"", pos);
        if (kid_pos == std::string::npos || kid_pos > arr_start) break;

        std::string kid = json_get_string(jwks_json.substr(kid_pos), "kid");
        if (kid.empty()) { pos = kid_pos + 1; continue; }

        auto n_opt = json_get_string_opt(jwks_json.substr(kid_pos), "n");
        auto e_opt = json_get_string_opt(jwks_json.substr(kid_pos), "e");

        if (n_opt && e_opt) {
            keys[kid] = {{"n", *n_opt}, {"e", *e_opt}};
        }

        pos = kid_pos + 1;
    }

    return keys;
}

std::string GoogleJWTVerifier::extract_header(const std::string& jwt) {
    std::vector<std::string> parts;
    std::istringstream iss(jwt);
    std::string part;
    while (std::getline(iss, part, '.')) parts.push_back(part);
    return base64url_decode(parts[0]);
}

std::string GoogleJWTVerifier::extract_payload(const std::string& jwt) {
    std::vector<std::string> parts;
    std::istringstream iss(jwt);
    std::string part;
    while (std::getline(iss, part, '.')) parts.push_back(part);
    return base64url_decode(parts[1]);
}

bool GoogleJWTVerifier::verify_signature(const std::string& signing_input,
                                          const std::string& signature_b64url,
                                          const std::string& n,
                                          const std::string& e) {
    // Decode signature from base64url
    std::string sig_bytes = base64url_decode(url_to_standard(signature_b64url));

    // Build RSA public key from n, e (base64url-encoded)
    EVP_PKEY* pkey = nullptr;
    EVP_PKEY_CTX* ctx = nullptr;
    RSA* rsa = nullptr;

    // Convert base64url n, e to BIGNUMs
    // The n and e values are base64url-encoded integers
    std::string n_std = url_to_standard(n);
    std::string e_std = url_to_standard(e);

    unsigned char* n_bytes = nullptr;
    unsigned char* e_bytes = nullptr;
    int n_len = 0, e_len = 0;

    // Decode base64 to get raw bytes
    // For large base64, use a simple decode
    auto decode_base64 = [](const std::string& b64) -> std::vector<unsigned char> {
        std::string s = url_to_standard(b64);
        std::vector<unsigned char> out;
        size_t in_pos = 0;
        size_t in_len = s.size();
        while (in_pos < in_len) {
            int a = b64_find(s[in_pos++]);
            int b = b64_find(s[in_pos++]);
            int c = b64_find(s[in_pos++]);
            int d = b64_find(s[in_pos++]);
            int triplet = (a << 18) | (b << 12) | (c << 6) | d;
            if (a >= 0) out.push_back((uint8_t)((triplet >> 16) & 0xFF));
            if (b >= 0 && in_pos <= in_len) out.push_back((uint8_t)((triplet >> 8) & 0xFF));
            if (c >= 0 && in_pos <= in_len - 2) out.push_back((uint8_t)(triplet & 0xFF));
        }
        return out;
    };

    std::vector<unsigned char> n_raw = decode_base64(n);
    std::vector<unsigned char> e_raw = decode_base64(e);

    // Convert to BIGNUM
    BIGNUM* bn_n = BN_bin2bn(n_raw.data(), (int)n_raw.size(), nullptr);
    BIGNUM* bn_e = BN_bin2bn(e_raw.data(), (int)e_raw.size(), nullptr);

    if (!bn_n || !bn_e) {
        BN_free(bn_n);
        BN_free(bn_e);
        return false;
    }

    rsa = RSA_new();
    rsa->n = bn_n;
    rsa->e = bn_e;

    pkey = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(pkey, rsa);

    // Verify the signature (public key operation)
     EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
     EVP_DigestVerifyInit(md_ctx, nullptr, EVP_sha256(), nullptr, pkey);
     EVP_DigestVerifyUpdate(md_ctx, signing_input.data(), signing_input.size());

     int verified = EVP_DigestVerifyFinal(md_ctx,
                                           (const unsigned char*)sig_bytes.data(),
                                           sig_bytes.size());
     bool success = (verified == 1);

     EVP_MD_CTX_free(md_ctx);
    return verified;
}
