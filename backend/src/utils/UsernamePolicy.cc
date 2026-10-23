#include "UsernamePolicy.h"

#include <algorithm>

namespace username_policy {

namespace {

bool isAsciiAlnum(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
}

bool isAllowed(char c) {
    return isAsciiAlnum(c) || c == '_' || c == '-' || c == '.';
}

bool startsWithNoCase(const std::string& s, const std::string& prefix) {
    return s.size() >= prefix.size() && Lower(s.substr(0, prefix.size())) == prefix;
}

}  // namespace

std::string Lower(const std::string& s) {
    std::string out(s);
    for (char& c : out) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return out;
}

std::string ShapeError(const std::string& name) {
    if (name.find('@') != std::string::npos) {
        return "Usernames can't contain @. To use your email, sign in with Google.";
    }
    if (startsWithNoCase(name, kVisitorPrefix)) {
        return "Names starting with \"visitor-\" are reserved. Please choose another.";
    }
    if (startsWithNoCase(name, kDeletedPrefix)) {
        return "Names starting with \"deleted-\" are reserved. Please choose another.";
    }
    if (name.size() < kMinLength || name.size() > kMaxLength) {
        return "Usernames are 3 to 24 characters long.";
    }
    for (char c : name) {
        if (!isAllowed(c)) {
            return "Use only letters, digits, and _ - . in a username.";
        }
    }
    if (!isAsciiAlnum(name[0])) {
        return "A username starts with a letter or a digit.";
    }
    return "";
}

std::string Check(const std::string& name, const std::function<bool(const std::string&)>& taken) {
    const std::string shape = ShapeError(name);
    if (!shape.empty()) return shape;
    if (taken && taken(name)) return "That username is taken. Please choose another.";
    return "";
}

std::string DeletedName(int userId) {
    return std::string(kDeletedPrefix) + std::to_string(userId);
}

bool IsDeletedName(const std::string& name) {
    const std::string prefix = kDeletedPrefix;
    if (name.size() <= prefix.size() || name.rfind(prefix, 0) != 0) return false;
    for (size_t i = prefix.size(); i < name.size(); ++i) {
        if (name[i] < '0' || name[i] > '9') return false;
    }
    return true;
}

std::string DisplayName(const std::string& name) {
    return IsDeletedName(name) ? kDeletedDisplayName : name;
}

std::string ScrubDeletedNames(const std::string& text) {
    const std::string prefix = kDeletedPrefix;
    if (text.find(prefix) == std::string::npos) return text;
    std::string out;
    out.reserve(text.size());
    size_t i = 0;
    while (i < text.size()) {
        const size_t at = text.find(prefix, i);
        if (at == std::string::npos) {
            out.append(text, i, std::string::npos);
            break;
        }
        size_t end = at + prefix.size();
        while (end < text.size() && text[end] >= '0' && text[end] <= '9') ++end;
        // A whole name only: digits after the prefix, and not the tail of a
        // longer word ("undeleted-3") or the head of one ("deleted-3a").
        const bool digits = end > at + prefix.size();
        const bool startsWord = at == 0 || !isAllowed(text[at - 1]) || text[at - 1] == '.';
        const bool endsWord = end == text.size() || !isAsciiAlnum(text[end]);
        out.append(text, i, at - i);
        if (digits && startsWord && endsWord) {
            out += kDeletedDisplayName;
        } else {
            out.append(text, at, end - at);
        }
        i = end;
    }
    return out;
}

std::string BaseFromEmail(const std::string& email) {
    std::string local = email.substr(0, email.find('@'));
    std::string base;
    for (char c : local) {
        if (isAllowed(c)) {
            base += c;
        } else if (c == '+' || c == ' ') {
            base += '_';
        }
        // Anything else (non-ASCII, quotes, ...) is dropped.
    }
    // Starts with a letter or a digit.
    size_t first = 0;
    while (first < base.size() && !isAsciiAlnum(base[first])) ++first;
    base = base.substr(first);
    if (base.empty()) base = "user";
    // Off the reserved prefixes ("visitor-x@..." would otherwise be refused).
    if (startsWithNoCase(base, kVisitorPrefix) || startsWithNoCase(base, kDeletedPrefix)) {
        base = "u" + base;
    }
    if (base.size() > kMaxLength) base = base.substr(0, kMaxLength);
    while (!base.empty() && !isAsciiAlnum(base.back()) && base.size() > kMinLength) base.pop_back();
    if (base.size() < kMinLength) base += std::string(kMinLength - base.size(), '0');
    return base;
}

std::string MakeUnique(const std::string& base, const std::function<bool(const std::string&)>& taken) {
    if (!taken || !taken(base)) return base;
    for (int n = 2; n < 100000; ++n) {
        const std::string suffix = std::to_string(n);
        std::string stem = base.substr(0, std::min(base.size(), kMaxLength - suffix.size()));
        const std::string candidate = stem + suffix;
        if (!taken(candidate)) return candidate;
    }
    return base;  // unreachable in practice
}

}  // namespace username_policy
