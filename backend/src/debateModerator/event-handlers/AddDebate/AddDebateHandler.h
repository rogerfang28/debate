#pragma once
#include <string>
#include "../../../utils/DebateWrapper.h"

class AddDebateHandler {
public:
    static void AddDebate(const std::string& debate_topic, const std::string& user, DebateWrapper& debateWrapper);
};