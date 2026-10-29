#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>
#include <vector>

#include "AnonymousUser.h"

// Who may use the admin-only features (today: importing a whole debate).
//
// DEBATE_ADMINS is a comma-separated list of usernames, matched without
// regard to case and with spaces around each name ignored:
//
//     DEBATE_ADMINS=roger,phase2-tester
//
// Unset or empty means nobody is an admin. A visitor is never one, whatever
// the list says. The variable is read on every call, so the list is whatever
// the server was started with -- there is no admin flag stored on a user.
namespace admins {

inline std::string Lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

inline std::string Trim(const std::string& s) {
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

// The names in a DEBATE_ADMINS value, lower-cased, blanks dropped.
inline std::vector<std::string> ParseList(const std::string& value) {
    std::vector<std::string> names;
    size_t start = 0;
    while (start <= value.size()) {
        const size_t comma = value.find(',', start);
        const std::string name = Lower(Trim(value.substr(start, comma == std::string::npos ? std::string::npos
                                                                                           : comma - start)));
        if (!name.empty()) names.push_back(name);
        if (comma == std::string::npos) break;
        start = comma + 1;
    }
    return names;
}

// Whether `username` is on the list in `value` (a DEBATE_ADMINS value).
inline bool IsAdminIn(const std::string& value, const std::string& username) {
    const std::string wanted = Lower(Trim(username));
    if (wanted.empty() || anonymous_user::IsAnonymousUsername(wanted)) return false;
    const std::vector<std::string> names = ParseList(value);
    return std::find(names.begin(), names.end(), wanted) != names.end();
}

// Whether `username` is an admin on this server.
inline bool IsAdmin(const std::string& username) {
    const char* value = std::getenv("DEBATE_ADMINS");
    return value != nullptr && IsAdminIn(value, username);
}

}  // namespace admins
