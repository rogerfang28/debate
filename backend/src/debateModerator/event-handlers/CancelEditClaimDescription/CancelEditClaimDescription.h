#pragma once
#include <string>
#include "../../../utils/DebateWrapper.h"
class CancelEditClaimDescriptionHandler {
public:
    static void CancelEditClaimDescription(const std::string& user, DebateWrapper& debateWrapper);
};