#pragma once
#include <string>
#include "../../../utils/DebateWrapper.h"

class DeleteDebateHandler {
public:
    static void DeleteDebate(const std::string& debate_id, const std::string& user, DebateWrapper& debateWrapper);
};