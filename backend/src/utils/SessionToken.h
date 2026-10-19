#pragma once

#include <cstdint>
#include <optional>
#include <string>

// The session cookie's value: who the reader is, until when, and a signature
// only this server can make.
//
// The cookie used to be two plain values, the user's number and name, and the
// server believed them whenever they matched. Both are public (names show on
// every claim, numbers count up from 1), so anyone could set them by hand and
// become anyone. Now the value is signed with HMAC-SHA256 under a secret that
// never leaves the server: a cookie this server did not issue, or one that has
// been edited or has expired, verifies as no session at all.
//
//   v1.<user id>.<username as hex>.<expires, unix seconds>.<signature as hex>
//
// The signature covers everything before it. The username is hex so that no
// character in a name can break the format. The expiry is inside the signed
// part, so a copied cookie stops working on its own; the cookie itself stays
// a browser-session cookie (see /privacy.html).
namespace session_token {

struct Session {
    int userId = 0;
    std::string username;
};

// How long a signed-in session lasts before it must be signed again.
constexpr int64_t kLifetimeSeconds = 30LL * 24 * 60 * 60;

// The shortest secret accepted from the environment, in characters.
constexpr size_t kMinSecretLength = 32;

std::string Sign(int userId, const std::string& username, int64_t expiresAt, const std::string& secret);

// The session the token names, if it is well formed, signed with `secret`,
// and not expired at `now`. Anything else is nullopt.
std::optional<Session> Verify(const std::string& token, int64_t now, const std::string& secret);

// The server's signing secret: DEBATE_SESSION_SECRET when it is set and long
// enough, otherwise a random one made once for this process (sessions then
// end when the server restarts). Read once, at first use.
const std::string& ServerSecret();

// Whether DEBATE_SESSION_SECRET is set and long enough. main() refuses to
// start a production server (DEBATE_ENV=production) without it.
bool SecretIsConfigured();

int64_t NowSeconds();

}  // namespace session_token
