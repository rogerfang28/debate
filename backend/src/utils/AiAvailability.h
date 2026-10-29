#pragma once

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <string>

// Whether this server offers the AI features: the chat beside every debate
// and building a debate from a passage. On by default. A server that cannot
// reach a language model (the public site, whose host is not on the network
// the model runs on) sets DEBATE_AI=off: the pages then leave the chat and the
// AI form out, and the moderator ignores the AI events, so nobody waits ten
// seconds for an error. The computed Brief needs no model and stays.
namespace ai_availability {

inline bool Enabled() {
    const char* value = std::getenv("DEBATE_AI");
    if (value == nullptr || value[0] == '\0') return true;
    std::string v(value);
    std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return !(v == "off" || v == "0" || v == "false" || v == "no");
}

}  // namespace ai_availability
