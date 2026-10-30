#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

#include "Admins.h"

// Who this server offers the AI features to: the chat beside every debate
// and building a debate from a passage. DEBATE_AI says:
//
//     (unset), on   everyone
//     admins        only the names in DEBATE_ADMINS (utils/Admins.h) -- the
//                   public site while its one model, which answers one
//                   question at a time, is tried out
//     off           nobody: a server that cannot reach a language model
//
// For whoever it is off, the pages leave the chat and the AI form out, and
// the moderator ignores the AI events, so nobody waits ten seconds for an
// error. The computed Brief needs no model and stays.
namespace ai_availability {

enum class Mode { Everyone, AdminsOnly, Nobody };

inline Mode CurrentMode() {
    const char* value = std::getenv("DEBATE_AI");
    if (value == nullptr || value[0] == '\0') return Mode::Everyone;
    std::string v(value);
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (v == "off" || v == "0" || v == "false" || v == "no") return Mode::Nobody;
    if (v == "admins" || v == "admin") return Mode::AdminsOnly;
    return Mode::Everyone;
}

// For a viewer already known to be (or not be) an admin.
inline bool EnabledFor(bool viewerIsAdmin) {
    const Mode mode = CurrentMode();
    return mode == Mode::Everyone || (mode == Mode::AdminsOnly && viewerIsAdmin);
}

// For a viewer by name; a visitor is never an admin.
inline bool EnabledFor(const std::string& username) {
    const Mode mode = CurrentMode();
    if (mode != Mode::AdminsOnly) return mode == Mode::Everyone;
    return admins::IsAdmin(username);
}

}  // namespace ai_availability
