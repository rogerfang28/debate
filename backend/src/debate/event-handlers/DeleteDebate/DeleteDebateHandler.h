#pragma once
#include <string>

class DeleteDebateHandler {
public:
    static void DeleteDebate(const std::string& debate_id, const std::string& user);
};