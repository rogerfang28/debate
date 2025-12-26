#pragma once

#include <string>
#include "../../../utils/DebateWrapper.h"

class GoHomeHandler {
public:
    static bool GoHome(const std::string& user, DebateWrapper& debateWrapper);
};