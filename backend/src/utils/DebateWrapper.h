#pragma once

#include "../../../../../src/gen/cpp/debate.pb.h"
#include <vector>
#include <string>

class DebateWrapper {
public:
    explicit DebateWrapper(const debate::Debate& debate);
    std::vector<debate::Claim> findChildren(const std::string& parentId) const;
    debate::Claim findClaim(const std::string& debate_id, const std::string& claimId) const;
    void createNewDebate(const std::string& topic, const std::string& owner);
    void findStatementParent(const std::string& statementId);
    void addStatementUnderParent(
        const std::string& parentId, 
        const std::string& statementText, 
        const std::string& connectionToParent);
    void changeDescriptionOfStatement(
        const std::string& statementId,
        const std::string& newDescription);
    void editStatementText(
        const std::string& statementId,
        const std::string& newText);
    

private:
    const debate::Debate& debate;
};