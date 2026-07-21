#include "parseCookie.h"
#include <sstream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <algorithm>
#include "Log.h"

// ============================================================
// HMAC-SHA256 Implementation (RFC 2104)
// ============================================================

// SHA-256 helper functions (standalone, no external dependency)

static const uint32_t sha256k[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,
    0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,
    0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,
    0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,
    0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
    0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,
    0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x27b70a85,0x2e1b2138,
    0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,
    0x81c2c92e,0x92722c85,0xa2bfe8a1,0xa81a664b,
    0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,
    0xf40e3585,0x106aa070,0x19a4c116,0x1e376c08,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,
    0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3
};

static inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
static inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
static inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
static inline uint32_t ep0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
static inline uint32_t ep1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
static inline uint32_t sig0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
static inline uint32_t sig1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

static void sha256_transform(uint32_t state[8], const uint8_t block[64]) {
    uint32_t w[64];
    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i*4] << 24) | ((uint32_t)block[i*4+1] << 16) |
               ((uint32_t)block[i*4+2] << 8) | (uint32_t)block[i*4+3];
    }
    for (int i = 16; i < 64; i++) {
        w[i] = sig1(w[i-2]) + w[i-7] + sig0(w[i-15]) + w[i-16];
    }
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + ep1(e) + ch(e, f, g) + sha256k[i] + w[i];
        uint32_t t2 = ep0(a) + maj(a, b, c);
        h = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    state[0] += a; state[1] += b; state[2] += c; state[3] += d;
    state[4] += e; state[5] += f; state[6] += g; state[7] += h;
}

static void sha256(const uint8_t* data, size_t len, uint8_t out[32]) {
    static const uint64_t sha256init[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    uint32_t state[8];
    for (int i = 0; i < 8; i++) state[i] = (uint32_t)sha256init[i];

    size_t total_len = len + 1;  // +1 for padding byte 0x80
    size_t padded_len = ((total_len + 7) / 64 + 1) * 64;
    // Round up to multiple of 64, at least 64+8
    if (padded_len < 64 + 8) padded_len = 64 + 8;

    uint8_t* final_pad = new uint8_t[padded_len];
    memset(final_pad, 0, padded_len);
    memcpy(final_pad, data, len);
    final_pad[len] = 0x80;
    // Append original length in bits (big-endian)
    uint64_t bits = len * 8;
    final_pad[padded_len - 8] = (uint8_t)(bits >> 56);
    final_pad[padded_len - 7] = (uint8_t)(bits >> 48);
    final_pad[padded_len - 6] = (uint8_t)(bits >> 40);
    final_pad[padded_len - 5] = (uint8_t)(bits >> 32);
    final_pad[padded_len - 4] = (uint8_t)(bits >> 24);
    final_pad[padded_len - 3] = (uint8_t)(bits >> 16);
    final_pad[padded_len - 2] = (uint8_t)(bits >> 8);
    final_pad[padded_len - 1] = (uint8_t)bits;

    for (size_t i = 0; i < padded_len; i += 64) {
        sha256_transform(state, final_pad + i);
    }
    delete[] final_pad;
    for (int i = 0; i < 8; i++) {
        out[i*4]   = (uint8_t)(state[i] >> 24);
        out[i*4+1] = (uint8_t)(state[i] >> 16);
        out[i*4+2] = (uint8_t)(state[i] >> 8);
        out[i*4+3] = (uint8_t)state[i];
    }
}

std::string HMAC::sign(const std::string& data, const std::string& key) {
    uint8_t key_block[64] = {0};
    size_t key_len = key.size();
    if (key_len > 64) {
        uint8_t hashed_key[32];
        sha256((const uint8_t*)key.data(), key_len, hashed_key);
        memcpy(key_block, hashed_key, 32);
    } else {
        memcpy(key_block, key.data(), key_len);
    }
    uint8_t opad[64], ipad[64];
    for (int i = 0; i < 64; i++) {
        opad[i] = key_block[i] ^ 0x5c;
        ipad[i] = key_block[i] ^ 0x36;
    }
    // Inner hash: H(ipad || data)
    uint8_t inner_hash[32];
    size_t inner_size = 64 + data.size();
    uint8_t* inner_data = new uint8_t[inner_size];
    memcpy(inner_data, ipad, 64);
    memcpy(inner_data + 64, data.data(), data.size());
    sha256(inner_data, inner_size, inner_hash);
    delete[] inner_data;
    // Outer hash: H(opad || inner_hash)
    uint8_t outer_data[64 + 32];
    memcpy(outer_data, opad, 64);
    memcpy(outer_data + 64, inner_hash, 32);
    uint8_t result[32];
    sha256(outer_data, 96, result);
    // Return hex-encoded signature
    std::ostringstream oss;
    for (int i = 0; i < 32; i++) oss << std::hex << std::setw(2) << std::setfill('0') << (int)result[i];
    return oss.str();
}

bool HMAC::verify(const std::string& data, const std::string& sig, const std::string& key) {
    return sig == sign(data, key);
}

std::string HMAC::getSecretKey() {
    const char* env = std::getenv("DEBATE_COOKIE_SECRET");
    if (env && env[0] != '\0') return std::string(env);
    // Fallback — WARNING: must be overridden in production via env var
    return "DEBATE_TOOL_DEFAULT_COOKIE_SECRET_CHANGE_ME_2026";
}

// ============================================================
// Base64 utility functions
// ============================================================

std::string parseCookie::base64Encode(const std::vector<unsigned char>& input) {
    static const char table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    size_t i = 0;
    uint8_t buf[3];
    size_t len = input.size();
    while (i < len) {
        size_t remaining = std::min<size_t>(3, len - i);
        memset(buf, 0, 3);
        for (size_t j = 0; j < remaining; j++) buf[j] = input[i + j];
        result += table[(buf[0] & 0xFC) >> 2];
        result += table[((buf[0] & 0x03) << 4) | ((buf[1] & 0xF0) >> 4)];
        if (remaining > 1) {
            result += table[((buf[1] & 0x0F) << 2) | ((buf[2] & 0xC0) >> 6)];
        } else {
            result += '=';
        }
        if (remaining > 2) {
            result += table[buf[2] & 0x3F];
        } else {
            result += '=';
        }
        i += 3;
    }
    return result;
}

std::vector<unsigned char> parseCookie::base64Decode(const std::string& input) {
    static const int table[] = {
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,62,-1,-1,-1,63,
        52,53,54,55,56,57,58,59,60,61,-1,-1,-1,-1,-1,-1,
        -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,
        15,16,17,18,19,20,21,22,23,24,25,-1,-1,-1,-1,-1,
        -1,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,
        41,42,43,44,45,46,47,48,49,50,51,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
        -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1
    };
    std::vector<unsigned char> result;
    size_t i = 0;
    while (i < input.size() && input[i] != '=') {
        size_t remaining = input.size() - i;
        if (remaining < 4) break;
        int b[4];
        for (int j = 0; j < 4; j++) {
            unsigned char c = input[i + j];
            b[j] = (c >= 128) ? -1 : table[c];
        }
        if (b[0] < 0 || b[1] < 0) break;
        result.push_back((uint8_t)((b[0] << 2) | (b[1] >> 4)));
        if (b[2] >= 0 && input[i + 2] != '=') {
            result.push_back((uint8_t)((b[1] << 4) | (b[2] >> 2)));
        }
        if (b[3] >= 0 && input[i + 3] != '=') {
            result.push_back((uint8_t)((b[2] << 6) | b[3]));
        }
        i += 4;
    }
    return result;
}

// ============================================================
// Cookie signing and extraction
// ============================================================

std::string parseCookie::signUserData(int user_id, const std::string& username) {
    std::string data = std::to_string(user_id) + ":" + username;
    std::string sig = HMAC::sign(data, HMAC::getSecretKey());
    std::string payload = data + "|" + sig;
    return base64Encode(reinterpret_cast<const unsigned char*>(payload.data()), payload.size());
}

std::string parseCookie::verifyCookieData(const std::string& signed_value) {
    auto decoded = base64Decode(signed_value);
    if (decoded.size() < 3) return "";
    std::string payload(decoded.begin(), decoded.end());
    size_t pos = payload.find_last_of('|');
    if (pos == std::string::npos) return "";
    std::string data = payload.substr(0, pos);
    std::string sig = payload.substr(pos + 1);
    if (!HMAC::verify(data, sig, HMAC::getSecretKey())) {
        Log::debug("[Auth] HMAC verification failed — cookie tampered");
        return "";
    }
    return data;
}

int parseCookie::extractUserIdFromCookies(const httplib::Request& req) {
    auto cookie_header = req.get_header_value("Cookie");
    if (cookie_header.empty()) {
        Log::debug("[Auth] No cookie found, returning 0 (guest)");
        return 0;
    }

    std::string prefix = "user_data=";
    size_t pos = cookie_header.find(prefix);
    if (pos == std::string::npos) {
        // Also try the old format for backward compatibility during transition
        int fallback = 0;
        std::string old_prefix = "user_id=";
        size_t old_pos = cookie_header.find(old_prefix);
        if (old_pos != std::string::npos) {
            size_t start = old_pos + old_prefix.length();
            size_t end = cookie_header.find(';', start);
            std::string userIdStr = (end == std::string::npos)
                ? cookie_header.substr(start)
                : cookie_header.substr(start, end - start);
            try {
                fallback = std::stoi(userIdStr);
                Log::debug("[Auth] Fallback: raw user_id from legacy cookie: " + std::to_string(fallback));
            } catch (...) {
                Log::error("[Auth] Failed to parse legacy user_id: " + userIdStr);
                return 0;
            }
        }
        return fallback;
    }

    size_t start = pos + prefix.length();
    size_t end = cookie_header.find(';', start);
    std::string encoded = (end == std::string::npos)
        ? cookie_header.substr(start)
        : cookie_header.substr(start, end - start);

    std::string data = verifyCookieData(encoded);
    if (data.empty()) {
        Log::error("[Auth] Cookie signature invalid — potential tampering");
        return 0;
    }

    // Parse "user_id:username"
    size_t colon = data.find(':');
    if (colon == std::string::npos) return 0;
    try {
        int userId = std::stoi(data.substr(0, colon));
        Log::debug("[Auth] Extracted verified user_id: " + std::to_string(userId));
        return userId;
    } catch (...) {
        Log::error("[Auth] Failed to parse verified user_id from: " + data);
        return 0;
    }
}

std::string parseCookie::extractUsernameFromCookies(const httplib::Request& req) {
    auto cookie_header = req.get_header_value("Cookie");
    if (cookie_header.empty()) return "";

    std::string prefix = "user_data=";
    size_t pos = cookie_header.find(prefix);
    if (pos == std::string::npos) {
        // Fallback to legacy cookie
        std::string old_prefix = "username=";
        size_t old_pos = cookie_header.find(old_prefix);
        if (old_pos == std::string::npos) return "";
        size_t start = old_pos + old_prefix.length();
        size_t end = cookie_header.find(';', start);
        std::string username = (end == std::string::npos)
            ? cookie_header.substr(start)
            : cookie_header.substr(start, end - start);
        return username;
    }

    size_t start = pos + prefix.length();
    size_t end = cookie_header.find(';', start);
    std::string encoded = (end == std::string::npos)
        ? cookie_header.substr(start)
        : cookie_header.substr(start, end - start);

    std::string data = verifyCookieData(encoded);
    if (data.empty()) return "";

    size_t colon = data.find(':');
    if (colon == std::string::npos) return "";
    return data.substr(colon + 1);
}

void parseCookie::setCookieUsername(httplib::Response& res, std::string username) {
    std::string cookie_value = "username=" + username + "; Path=/; HttpOnly; SameSite=Lax";
    res.set_header("Set-Cookie", cookie_value);
}

void parseCookie::setCookieUserId(httplib::Response& res, int user_id) {
    std::string cookie_value = "user_id=" + std::to_string(user_id) + "; Path=/; HttpOnly; SameSite=Lax";
    res.set_header("Set-Cookie", cookie_value);
}

void parseCookie::clearAuthCookies(httplib::Response& res) {
    std::string expired = "user_id=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly; SameSite=Lax";
    res.set_header("Set-Cookie", expired);
    expired = "username=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly; SameSite=Lax";
    res.set_header("Set-Cookie", expired);
    expired = "user_data=; Path=/; Expires=Thu, 01 Jan 1970 00:00:00 GMT; HttpOnly; SameSite=Lax";
    res.set_header("Set-Cookie", expired);
}
