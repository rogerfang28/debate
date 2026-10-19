#include "SessionToken.h"
#include "Log.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include <chrono>
#include <cstdlib>
#include <stdexcept>
#include <vector>

namespace session_token {
namespace {

constexpr const char* kVersion = "v1";

std::string toHex(const unsigned char* data, size_t len) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(digits[data[i] >> 4]);
        out.push_back(digits[data[i] & 0x0f]);
    }
    return out;
}

int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return -1;  // upper case is never issued, so it is never accepted
}

std::optional<std::string> fromHex(const std::string& hex) {
    if (hex.size() % 2 != 0) return std::nullopt;
    std::string out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i < hex.size(); i += 2) {
        const int hi = hexValue(hex[i]), lo = hexValue(hex[i + 1]);
        if (hi < 0 || lo < 0) return std::nullopt;
        out.push_back(static_cast<char>((hi << 4) | lo));
    }
    return out;
}

// Digits only, no sign, no leading zero, fits: the exact text Sign() writes.
std::optional<int64_t> parseCount(const std::string& text) {
    if (text.empty() || text.size() > 18) return std::nullopt;
    if (text.size() > 1 && text[0] == '0') return std::nullopt;
    int64_t value = 0;
    for (char c : text) {
        if (c < '0' || c > '9') return std::nullopt;
        value = value * 10 + (c - '0');
    }
    return value;
}

std::string mac(const std::string& payload, const std::string& secret) {
    unsigned char out[EVP_MAX_MD_SIZE];
    unsigned int outLen = 0;
    const unsigned char* result = HMAC(
        EVP_sha256(), secret.data(), static_cast<int>(secret.size()),
        reinterpret_cast<const unsigned char*>(payload.data()), payload.size(), out, &outLen);
    if (result == nullptr) {
        throw std::runtime_error("session_token: HMAC failed");
    }
    return toHex(out, outLen);
}

std::string payloadFor(int userId, const std::string& usernameHex, int64_t expiresAt) {
    return std::string(kVersion) + "." + std::to_string(userId) + "." + usernameHex + "." +
           std::to_string(expiresAt);
}

std::vector<std::string> split(const std::string& s, char sep) {
    std::vector<std::string> parts;
    size_t start = 0;
    while (true) {
        const size_t at = s.find(sep, start);
        parts.push_back(s.substr(start, at == std::string::npos ? std::string::npos : at - start));
        if (at == std::string::npos) break;
        start = at + 1;
    }
    return parts;
}

const char* envSecret() {
    const char* value = std::getenv("DEBATE_SESSION_SECRET");
    return (value != nullptr && std::string(value).size() >= kMinSecretLength) ? value : nullptr;
}

}  // namespace

std::string Sign(int userId, const std::string& username, int64_t expiresAt, const std::string& secret) {
    const std::string usernameHex =
        toHex(reinterpret_cast<const unsigned char*>(username.data()), username.size());
    const std::string payload = payloadFor(userId, usernameHex, expiresAt);
    return payload + "." + mac(payload, secret);
}

std::optional<Session> Verify(const std::string& token, int64_t now, const std::string& secret) {
    const std::vector<std::string> parts = split(token, '.');
    if (parts.size() != 5 || parts[0] != kVersion) return std::nullopt;

    const auto userId = parseCount(parts[1]);
    const auto username = fromHex(parts[2]);
    const auto expiresAt = parseCount(parts[3]);
    if (!userId || *userId <= 0 || *userId > INT32_MAX || !username || username->empty() || !expiresAt) {
        return std::nullopt;
    }

    // The signature is checked before the expiry, and in constant time, so
    // nothing about a forged token's signature leaks through timing.
    const std::string expected = mac(payloadFor(static_cast<int>(*userId), parts[2], *expiresAt), secret);
    const std::string& given = parts[4];
    if (given.size() != expected.size() ||
        CRYPTO_memcmp(given.data(), expected.data(), expected.size()) != 0) {
        return std::nullopt;
    }
    if (now >= *expiresAt) return std::nullopt;

    return Session{static_cast<int>(*userId), *username};
}

bool SecretIsConfigured() {
    return envSecret() != nullptr;
}

const std::string& ServerSecret() {
    static const std::string secret = [] {
        if (const char* value = envSecret()) {
            return std::string(value);
        }
        if (const char* value = std::getenv("DEBATE_SESSION_SECRET"); value != nullptr && value[0] != '\0') {
            Log::warn("[Session] DEBATE_SESSION_SECRET is shorter than " + std::to_string(kMinSecretLength) +
                      " characters; ignoring it.");
        }
        unsigned char bytes[32];
        if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
            throw std::runtime_error("session_token: could not generate a random secret");
        }
        Log::warn("[Session] No DEBATE_SESSION_SECRET; using a random secret for this run. "
                  "Everyone is signed out when the server restarts.");
        return toHex(bytes, sizeof(bytes));
    }();
    return secret;
}

int64_t NowSeconds() {
    return std::chrono::duration_cast<std::chrono::seconds>(
               std::chrono::system_clock::now().time_since_epoch())
        .count();
}

}  // namespace session_token
