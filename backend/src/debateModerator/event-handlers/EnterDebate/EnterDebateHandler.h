#pragma once

#include <string>
#include "../../../utils/DebateWrapper.h"

class EnterDebateHandler {
public:
    static bool EnterDebate(const std::string& topicID, const std::string& user, DebateWrapper& debateWrapper);
};
