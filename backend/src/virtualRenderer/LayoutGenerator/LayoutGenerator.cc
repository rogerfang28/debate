#include "LayoutGenerator.h"

// #include "./pageGenerator.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../pages/generators/homePage/HomePageGenerator.h"
#include "../pages/generators/debatePage/DebateClaimPageGenerator.h"
#include "../pages/generators/loginPage/LoginPageGenerator.h"

// #include "../utils/pathUtils.h"

ui::Page LayoutGenerator::generateLayout(const moderator_to_vr::ModeratorToVRMessage& info, const std::string& user) {
    // ok so we need to decode the info and generate a page based on it
    // so we should check user engagement first
    switch (info.engagement().current_action()){
        case user_engagement::ACTION_NONE:
            // generate home page
            std::cout << "[LayoutGenerator] Generating Home Page for user: " << user << "\n";

            // get the debate list from info
            if (info.has_users_debates()) {
                return HomePageGenerator::GenerateHomePage(info.users_debates());
            } else {
                debate::DebateList emptyList;
                return HomePageGenerator::GenerateHomePage(emptyList);
            }
        case user_engagement::ACTION_DEBATING:
            // generate debate page
            std::cout << "[LayoutGenerator] Generating Debate Page for user: " << user << "\n";
            if (info.has_debate()) {
                const auto& debate = info.debate();
                return DebateClaimPageGenerator::GenerateDebatePage(debate.topic(), "test");
            } else {
                std::cout << "[LayoutGenerator] No debate info found, generating Home Page instead.\n";
                debate::DebateList emptyList;
                return HomePageGenerator::GenerateHomePage(emptyList);
            }
        default:
            std::cout << "[LayoutGenerator] Unknown engagement action, generating Home Page by default.\n";
            debate::DebateList emptyList;
            return HomePageGenerator::GenerateHomePage(emptyList);
    }
    // Unreachable code - removed dead code below
}
