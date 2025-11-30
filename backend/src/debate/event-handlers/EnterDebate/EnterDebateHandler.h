#pragma once

#include <string>

class EnterDebateHandler {
public:
    static bool EnterDebate(const std::string& topicID, const std::string& user);
};
