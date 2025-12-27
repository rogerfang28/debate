#pragma once
#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/debate.pb.h"

class DebatePageGenerator {
public:
    static ui::Page GenerateDebatePage(
        const std::string& debateTopic, 
        const std::string& claim, 
        const std::string& currentClaimDescription, 
        std::vector<std::pair<std::string,std::string>> childClaimInfo, 
        bool openedAddChildClaimModal, 
        bool editingClaimDescription);
};