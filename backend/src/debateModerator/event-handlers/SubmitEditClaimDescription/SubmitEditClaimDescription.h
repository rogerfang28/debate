#pragma once
#include <string>
#include "../../../utils/DebateWrapper.h"
class SubmitEditClaimDescriptionHandler {
public:
    static void SubmitEditClaimDescription(const std::string& user, const std::string& newDescription, DebateWrapper& debateWrapper);
};