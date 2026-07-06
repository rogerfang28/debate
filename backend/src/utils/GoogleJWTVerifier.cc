#include "GoogleJWTVerifier.h"
#include "Log.h"
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
#include <vector>

namespace {
// Google's JWKS endpoint
const std::string JWKS_URL = "https://www.googleapis.com/oauth2/v3/certs";

// Cached JWKS keys: kid -> {n, e}
std::map<std::string, std::map<std::string, std::string>> g_jwks_cache;
std::chrono::system_clock::time_point g_jwks_cache_time;
const int JWKS_CACHE_SECONDS = 300; // 5 minutes

// Base64url decode table
const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int b64_find(char c) {
    for (int i = 0; i < 256; i++) {
        if (b64_table[i] == c) return i;
    }
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
    while (result.size() % 4 != 0) result += '=';
    return result;
}

// Decode base64 using the b64_find table
std::vector<unsigned char> base64_decode_internal(const std::string& b64) {
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
        if (a >= 0) out.push_back((unsigned char)((triplet >> 16) & 0xFF));
        if (b >= 0 && in_pos <= in_len) out.push_back((unsigned char)((triplet >> 8) & 0xFF));
        if (c >= 0 && in_pos <= in_len - 2) out.push_back((unsigned char)(triplet & 0xFF));
    }
    return out;
}

// Fetch JWKS from Google
std::string fetch_jwks() {
    httplib::Client cli("www.googleapis.com", 443);
    auto res = cli.Get("/oauth2/v3/certs");
    if (!res || res->status != 200) {
        throw std::runtime_error("Failed to fetch JWKS: " + (res ? std::to_string(res->status) : "no response"));
    }
    return res->body;
}

} // anonymous namespace

bool GoogleJWTVerifier::init() {
    try {
        std::string jwks = fetch_jwks();
        // Parse JWKS inline (same logic as parse_jwks, but static)
        g_jwks_cache.clear();
        size_t keys_pos = jwks.find("\"keys\"");
        if (keys_pos == std::string::npos) {
            Log::error("[GoogleAuth] No keys in JWKS response");
            return false;
        }
        size_t arr_start = jwks.find('[', keys_pos);
        if (arr_start == std::string::npos) {
            Log::error("[GoogleAuth] Invalid JWKS format");
            return false;
        }
        size_t pos = arr_start;
        while (pos < jwks.size()) {
            size_t kid_pos = jwks.find("\"kid\"", pos);
            if (kid_pos == std::string::npos || kid_pos > arr_start) break;
            std::string kid;
            size_t kid_q1 = jwks.find('"', kid_pos + 5);
            if (kid_q1 != std::string::npos) {
                size_t kid_q2 = jwks.find('"', kid_q1 + 1);
                if (kid_q2 != std::string::npos) {
                    kid = jwks.substr(kid_q1 + 1, kid_q2 - kid_q1 - 1);
                }
            }
            if (kid.empty()) { pos = kid_pos + 1; continue; }
            // Extract n
            std::string n_val, e_val;
            size_t n_pos = jwks.find("\"n\"", kid_pos);
            if (n_pos != std::string::npos) {
                size_t n_q1 = jwks.find('"', n_pos + 3);
                if (n_q1 != std::string::npos) {
                    size_t n_q2 = jwks.find('"', n_q1 + 1);
                    if (n_q2 != std::string::npos) {
                        n_val = jwks.substr(n_q1 + 1, n_q2 - n_q1 - 1);
                    }
                }
            }
            // Extract e
            size_t e_pos = jwks.find("\"e\"", kid_pos);
            if (e_pos != std::string::npos) {
                size_t e_q1 = jwks.find('"', e_pos + 3);
                if (e_q1 != std::string::npos) {
                    size_t e_q2 = jwks.find('"', e_q1 + 1);
                    if (e_q2 != std::string::npos) {
                        e_val = jwks.substr(e_q1 + 1, e_q2 - e_q1 - 1);
                    }
                }
            }
            if (!n_val.empty() && !e_val.empty()) {
                g_jwks_cache[kid] = {{"n", n_val}, {"e", e_val}};
            }
            pos = kid_pos + 1;
        }
        g_jwks_cache_time = std::chrono::system_clock::now();
        Log::info("[GoogleAuth] Loaded " + std::to_string(g_jwks_cache.size()) + " JWKS keys");
        return !g_jwks_cache.empty();
    } catch (const std::exception& e) {
        Log::error("[GoogleAuth] Failed to init JWKS: " + std::string(e.what()));
        return false;
    }
}

GoogleUserInfo GoogleJWTVerifier::verify(const std::string& id_token) {
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
        // Re-parse JWKS inline
        g_jwks_cache.clear();
        size_t keys_pos = jwks.find("\"keys\"");
        if (keys_pos != std::string::npos) {
            size_t arr_start = jwks.find('[', keys_pos);
            if (arr_start != std::string::npos) {
                size_t pos2 = arr_start;
                while (pos2 < jwks.size()) {
                    size_t kid_pos2 = jwks.find("\"kid\"", pos2);
                    if (kid_pos2 == std::string::npos || kid_pos2 > arr_start) break;
                    std::string kid2;
                    size_t kid_q1 = jwks.find('"', kid_pos2 + 5);
                    if (kid_q1 != std::string::npos) {
                        size_t kid_q2 = jwks.find('"', kid_q1 + 1);
                        if (kid_q2 != std::string::npos) {
                            kid2 = jwks.substr(kid_q1 + 1, kid_q2 - kid_q1 - 1);
                        }
                    }
                    if (kid2.empty()) { pos2 = kid_pos2 + 1; continue; }
                    std::string n_val, e_val;
                    size_t n_pos = jwks.find("\"n\"", kid_pos2);
                    if (n_pos != std::string::npos) {
                        size_t n_q1 = jwks.find('"', n_pos + 3);
                        if (n_q1 != std::string::npos) {
                            size_t n_q2 = jwks.find('"', n_q1 + 1);
                            if (n_q2 != std::string::npos) {
                                n_val = jwks.substr(n_q1 + 1, n_q2 - n_q1 - 1);
                            }
                        }
                    }
                    size_t e_pos = jwks.find("\"e\"", kid_pos2);
                    if (e_pos != std::string::npos) {
                        size_t e_q1 = jwks.find('"', e_pos + 3);
                        if (e_q1 != std::string::npos) {
                            size_t e_q2 = jwks.find('"', e_q1 + 1);
                            if (e_q2 != std::string::npos) {
                                e_val = jwks.substr(e_q1 + 1, e_q2 - e_q1 - 1);
                            }
                        }
                    }
                    if (!n_val.empty() && !e_val.empty()) {
                        g_jwks_cache[kid2] = {{"n", n_val}, {"e", e_val}};
                    }
                    pos2 = kid_pos2 + 1;
                }
            }
        }
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

    if (!rsa_verify(signing_input, signature_b64url, n, e)) {
        throw std::runtime_error("Signature verification failed");
    }

    // Decode and validate payload
    std::string payload_json = base64url_decode(parts[1]);

    // Validate iss (issuer)
    std::string token_iss = json_get_string(payload_json, "iss");
    if (token_iss != "accounts.google.com" && token_iss != "https://accounts.google.com") {
        throw std::runtime_error("Invalid iss: " + token_iss);
    }

    // Validate exp
    long long exp = json_get_int(payload_json, "exp");
    auto now_epoch = std::chrono::system_clock::to_time_t(now);
    if (exp <= (long long)now_epoch + 60) {
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

    while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r')) {
        pos++;
    }

    if (pos >= json.size() || json[pos] != '"') return std::nullopt;
    pos++;

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

void* GoogleJWTVerifier::b64url_to_bignum(const std::string& b64url) {
    // Decode base64url to raw bytes, then convert to BIGNUM
    std::vector<unsigned char> raw = base64_decode_internal(b64url);
    if (raw.empty()) return nullptr;
    BIGNUM* bn = BN_bin2bn(raw.data(), (int)raw.size(), nullptr);
    // Return opaque pointer — caller must BN_free it
    return (void*)bn;
}

bool GoogleJWTVerifier::rsa_verify(const std::string& signing_input,
                                    const std::string& signature_b64url,
                                    const std::string& n_b64url,
                                    const std::string& e_b64url) {
    // Decode signature from base64url
    std::vector<unsigned char> sig_bytes = base64_decode_internal(url_to_standard(signature_b64url));

    // Decode n, e from base64url to BIGNUMs
    std::vector<unsigned char> n_raw = base64_decode_internal(url_to_standard(n_b64url));
    std::vector<unsigned char> e_raw = base64_decode_internal(url_to_standard(e_b64url));

    if (n_raw.empty() || e_raw.empty()) {
        return false;
    }

    BIGNUM* bn_n = BN_bin2bn(n_raw.data(), (int)n_raw.size(), nullptr);
    BIGNUM* bn_e = BN_bin2bn(e_raw.data(), (int)e_raw.size(), nullptr);

    if (!bn_n || !bn_e) {
        BN_free(bn_n);
        BN_free(bn_e);
        return false;
    }

    RSA* rsa = RSA_new();
    RSA_set0_key(rsa, bn_n, bn_e, nullptr);
    // bn_n and bn_e now owned by rsa

    EVP_PKEY* pkey = EVP_PKEY_new();
    EVP_PKEY_assign_RSA(pkey, rsa);

    EVP_MD_CTX* md_ctx = EVP_MD_CTX_new();
    EVP_DigestVerifyInit(md_ctx, nullptr, EVP_sha256(), nullptr, pkey);
    EVP_DigestVerifyUpdate(md_ctx, signing_input.data(), (int)signing_input.size());
    int verified = EVP_DigestVerifyFinal(md_ctx, sig_bytes.data(), (int)sig_bytes.size());
    bool success = (verified == 1);

    EVP_MD_CTX_free(md_ctx);
    EVP_PKEY_free(pkey);
    return success;
}
