#pragma once

#include <string>

#include "DemoMode.h"

// A first-time visitor reads without signing in: the client makes up a name
// like "visitor-a1b2c3d4", keeps it in localStorage, and logs in with it
// silently. Each visitor is its own user record, so two of them never share a
// position (open debate, selected claim, tab). They may read everything and
// change nothing -- the moderator turns their mutating events into NONE and
// the layout generators leave the mutating buttons out.
//
// "guest" is deliberately NOT anonymous: it is the identity the scenario
// importer and the scenario tests enter data as (the Fauci debates among
// them), so making it read-only would block those writes.
namespace anonymous_user {

inline constexpr const char* kVisitorPrefix = "visitor-";

inline bool IsAnonymousUsername(const std::string& username) {
    return username.rfind(kVisitorPrefix, 0) == 0;
}

// The client-generated shape exactly: "visitor-" plus 8 lowercase hex digits.
// Anything else arriving as a visitor name is ignored.
inline bool IsWellFormedVisitorName(const std::string& username) {
    const std::string prefix = kVisitorPrefix;
    if (username.size() != prefix.size() + 8 || username.rfind(prefix, 0) != 0) {
        return false;
    }
    for (size_t i = prefix.size(); i < username.size(); ++i) {
        const char c = username[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }
    return true;
}

// The one question the layout generators ask: may this viewer see the
// buttons that change a debate? No in a read-only demo build, and no for an
// anonymous visitor.
inline bool IsReadOnlyViewer(const std::string& username) {
    return demo_mode::kReadOnlyMode || IsAnonymousUsername(username);
}

}  // namespace anonymous_user
