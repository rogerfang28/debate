#pragma once

#include <string>
#include "../../../utils/DebateWrapper.h"

class DebateHandler {
public:
    static void AddDebate(const std::string& debate_topic, const std::string& user, DebateWrapper& debateWrapper);
    static void AddDebate(const std::string& debate_topic, const int& user_id, DebateWrapper& debateWrapper);
    static void DeleteDebate(const std::string& debate_id, const int& user_id, DebateWrapper& debateWrapper);
    static void ClearDebates(const int& user_id, DebateWrapper& debateWrapper);
    static void JoinDebateAsMember(const std::string& debate_id, const int& user_id, DebateWrapper& debateWrapper);
};
