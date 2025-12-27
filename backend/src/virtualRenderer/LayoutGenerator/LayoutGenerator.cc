#include "LayoutGenerator.h"

#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "./pages/homePage/HomePageGenerator.h"
#include "./pages/debatePage/DebatePageGenerator.h"
#include "./pages/loginPage/LoginPageGenerator.h"
#include "./pages/errorPage/ErrorPageGenerator.h"
#include "../../utils/Log.h"

ui::Page LayoutGenerator::generateLayout(const moderator_to_vr::ModeratorToVRMessage& info, const std::string& user) {
    // ok so we need to decode the info and generate a page based on it
    // so we should check user engagement first
    switch (info.engagement().current_action()){
        case user_engagement::ACTION_NONE:
            // generate home page
            Log::debug("[LayoutGenerator] Generating Home Page for user: " + user);
            return HomePageGenerator::GenerateHomePage(info.engagement().none_info());
        case user_engagement::ACTION_DEBATING:
            // generate debate page
            Log::debug("[LayoutGenerator] Generating Debate Page for user: " + user);
            return DebatePageGenerator::GenerateDebatePage(info.engagement().debating_info());
        default:
            Log::debug("[LayoutGenerator] Unknown engagement action, generating error not found page with a go home button.");
            Log::debug("[LayoutGenerator] engagement action: " + std::to_string(info.engagement().current_action()));
            return ErrorPageGenerator::GenerateErrorPage();
    }
}
