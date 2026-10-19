#include <gtest/gtest.h>

#include "../../backend/src/utils/SessionToken.h"

#include <string>
#include <vector>

// ---------------------------------------
// SessionToken
// ---------------------------------------
// The session cookie is the whole of "who is this request": if a token that
// this server did not issue verifies, anyone can be anyone. So these tests are
// mostly about what must FAIL -- an edited user, an edited name, an edited
// expiry, a different secret, an expired token, and malformed text -- with one
// round trip to show that a genuine token still passes.

namespace {

using session_token::Sign;
using session_token::Verify;

const std::string kSecret = "0123456789abcdef0123456789abcdef";  // 32 characters
const std::string kOther  = "fedcba9876543210fedcba9876543210";
constexpr int64_t kNow = 1'800'000'000;
constexpr int64_t kLater = kNow + 3600;

std::vector<std::string> parts(const std::string& token) {
    std::vector<std::string> out;
    size_t start = 0;
    while (true) {
        const size_t at = token.find('.', start);
        out.push_back(token.substr(start, at == std::string::npos ? std::string::npos : at - start));
        if (at == std::string::npos) break;
        start = at + 1;
    }
    return out;
}

std::string join(const std::vector<std::string>& p) {
    std::string out;
    for (size_t i = 0; i < p.size(); ++i) out += (i ? "." : "") + p[i];
    return out;
}

}  // namespace

TEST(SessionToken, GenuineTokenVerifies) {
    const std::string token = Sign(42, "alice", kLater, kSecret);
    const auto session = Verify(token, kNow, kSecret);
    ASSERT_TRUE(session.has_value());
    EXPECT_EQ(session->userId, 42);
    EXPECT_EQ(session->username, "alice");
}

TEST(SessionToken, NamesWithAwkwardCharactersRoundTrip) {
    // Dots, semicolons, equals signs and non-ASCII would all break a naive
    // cookie or a dot-separated format; the name travels as hex.
    for (const std::string name : {"a.b", "x;y=z", "Ren\xC3\xA9e", "visitor-0a1b2c3d", "with space"}) {
        const auto session = Verify(Sign(7, name, kLater, kSecret), kNow, kSecret);
        ASSERT_TRUE(session.has_value()) << name;
        EXPECT_EQ(session->username, name);
    }
}

TEST(SessionToken, ChangingTheUserIdFails) {
    auto p = parts(Sign(42, "alice", kLater, kSecret));
    p[1] = "1";
    EXPECT_FALSE(Verify(join(p), kNow, kSecret).has_value());
}

TEST(SessionToken, ChangingTheUsernameFails) {
    auto p = parts(Sign(42, "alice", kLater, kSecret));
    p[2] = parts(Sign(42, "bob", kLater, kSecret))[2];
    EXPECT_FALSE(Verify(join(p), kNow, kSecret).has_value());
}

TEST(SessionToken, ExtendingTheExpiryFails) {
    auto p = parts(Sign(42, "alice", kLater, kSecret));
    p[3] = std::to_string(kLater + 365 * 24 * 3600);
    EXPECT_FALSE(Verify(join(p), kNow, kSecret).has_value());
}

TEST(SessionToken, ChangingTheSignatureFails) {
    auto p = parts(Sign(42, "alice", kLater, kSecret));
    p[4][0] = p[4][0] == 'a' ? 'b' : 'a';
    EXPECT_FALSE(Verify(join(p), kNow, kSecret).has_value());
}

TEST(SessionToken, ADifferentSecretFails) {
    EXPECT_FALSE(Verify(Sign(42, "alice", kLater, kOther), kNow, kSecret).has_value());
}

TEST(SessionToken, ExpiredTokenFails) {
    const std::string token = Sign(42, "alice", kLater, kSecret);
    EXPECT_TRUE(Verify(token, kLater - 1, kSecret).has_value());
    EXPECT_FALSE(Verify(token, kLater, kSecret).has_value());
    EXPECT_FALSE(Verify(token, kLater + 1, kSecret).has_value());
}

TEST(SessionToken, OldPlainCookieValuesFail) {
    // What the two cookies used to hold, and other things a hand-made
    // cookie might try.
    for (const std::string bad : {"42", "alice", "", "v1", "v1....", "v1.42.616c696365.1800003600",
                                  "v2.42.616c696365.1800003600.00",
                                  "v1.-42.616c696365.1800003600.00", "v1.042.616c696365.1800003600.00",
                                  "v1.42.ALICE.1800003600.00", "v1.42..1800003600.00"}) {
        EXPECT_FALSE(Verify(bad, kNow, kSecret).has_value()) << bad;
    }
}

TEST(SessionToken, ZeroOrNegativeUserIdIsNeverASession) {
    EXPECT_FALSE(Verify(Sign(0, "alice", kLater, kSecret), kNow, kSecret).has_value());
}
