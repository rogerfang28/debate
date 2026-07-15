#include "PasswordHasher.h"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <array>
#include <cstdint>
#include <sstream>
#include <vector>

namespace {

constexpr int kIterations = 100000;   // PBKDF2 rounds
constexpr int kSaltBytes = 16;
constexpr int kHashBytes = 32;        // SHA-256 output size

std::string to_hex(const unsigned char* data, size_t len) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    out.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        out.push_back(digits[data[i] >> 4]);
        out.push_back(digits[data[i] & 0x0F]);
    }
    return out;
}

std::vector<unsigned char> from_hex(const std::string& hex) {
    std::vector<unsigned char> out;
    if (hex.size() % 2 != 0) return out;
    out.reserve(hex.size() / 2);
    auto nibble = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = nibble(hex[i]);
        int lo = nibble(hex[i + 1]);
        if (hi < 0 || lo < 0) return {};
        out.push_back(static_cast<unsigned char>((hi << 4) | lo));
    }
    return out;
}

// Derive a PBKDF2-HMAC-SHA256 digest. Returns empty on failure.
std::vector<unsigned char> derive(const std::string& password,
                                  const unsigned char* salt, int salt_len,
                                  int iterations, int out_len) {
    std::vector<unsigned char> out(out_len);
    int ok = PKCS5_PBKDF2_HMAC(
        password.data(), static_cast<int>(password.size()),
        salt, salt_len,
        iterations,
        EVP_sha256(),
        out_len, out.data());
    if (ok != 1) return {};
    return out;
}

// Constant-time comparison to avoid leaking match position via timing.
bool constant_time_equal(const std::vector<unsigned char>& a,
                         const std::vector<unsigned char>& b) {
    if (a.size() != b.size()) return false;
    unsigned char diff = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        diff |= static_cast<unsigned char>(a[i] ^ b[i]);
    }
    return diff == 0;
}

}  // namespace

std::string PasswordHasher::hash(const std::string& password) {
    std::array<unsigned char, kSaltBytes> salt{};
    if (RAND_bytes(salt.data(), kSaltBytes) != 1) {
        return "";
    }

    std::vector<unsigned char> digest =
        derive(password, salt.data(), kSaltBytes, kIterations, kHashBytes);
    if (digest.empty()) {
        return "";
    }

    std::ostringstream oss;
    oss << "pbkdf2$" << kIterations << "$"
        << to_hex(salt.data(), kSaltBytes) << "$"
        << to_hex(digest.data(), digest.size());
    return oss.str();
}

bool PasswordHasher::verify(const std::string& password, const std::string& encoded) {
    // Expected format: pbkdf2$<iterations>$<salt_hex>$<hash_hex>
    std::vector<std::string> parts;
    std::string field;
    std::istringstream iss(encoded);
    while (std::getline(iss, field, '$')) {
        parts.push_back(field);
    }
    if (parts.size() != 4 || parts[0] != "pbkdf2") {
        return false;
    }

    int iterations = 0;
    try {
        iterations = std::stoi(parts[1]);
    } catch (...) {
        return false;
    }
    if (iterations <= 0) return false;

    std::vector<unsigned char> salt = from_hex(parts[2]);
    std::vector<unsigned char> expected = from_hex(parts[3]);
    if (salt.empty() || expected.empty()) {
        return false;
    }

    std::vector<unsigned char> actual =
        derive(password, salt.data(), static_cast<int>(salt.size()),
               iterations, static_cast<int>(expected.size()));
    if (actual.empty()) {
        return false;
    }

    return constant_time_equal(actual, expected);
}
