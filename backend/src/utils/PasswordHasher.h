#pragma once

#include <string>

// Password hashing using PBKDF2-HMAC-SHA256 (via OpenSSL).
//
// hash() produces a self-describing, salted digest string of the form:
//   "pbkdf2$<iterations>$<salt_hex>$<hash_hex>"
// which is stored directly in the user record's password_hash field.
// verify() re-derives the digest using the salt/iterations embedded in the
// stored string and compares in constant time.
class PasswordHasher {
public:
    // Hash a plaintext password. Generates a fresh random salt each call, so
    // hashing the same password twice yields different strings. Returns an
    // empty string only if secure random generation fails.
    static std::string hash(const std::string& password);

    // Verify a plaintext password against a previously produced hash string.
    // Returns false on any parse failure or mismatch.
    static bool verify(const std::string& password, const std::string& encoded);
};
