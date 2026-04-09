#include "LayoutGenerator.h"

#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/rendering_info.pb.h"
#include "./pages/homePage/HomePageGenerator.h"
#include "./pages/debatePage/DebatePageGenerator.h"
#include "./pages/debatePage/DebatePageInfoParser.h"
#include "./pages/loginPage/LoginPageGenerator.h"
#include "./pages/errorPage/ErrorPageGenerator.h"
#include "../../utils/Log.h"

ui::Page LayoutGenerator::generateLayout(const moderator_to_vr::ModeratorToVRMessage& info) {
    // ok so we need to decode the info and generate a page based on it
    // so we should check user engagement first
    const auto action = info.user().engagement().current_action();
    if (action == user_engagement::ACTION_HOME) {
        // generate home page
        Log::debug("[LayoutGenerator] Generating Home Page");
        return HomePageGenerator::GenerateHomePage(info.user());
    }

    if (action == user_engagement::ACTION_DEBATING) {
        // generate debate page
        Log::debug("[LayoutGenerator] Generating Debate Page");
        rendering_info::DebatePageRenderingInfo debatePageInfo = DebatePageInfoParser::ParseFromUser(info.user(), info.collection());
        return DebatePageGenerator::GenerateDebatePage(debatePageInfo);
    }

    if (action == user_engagement::ACTION_LOGIN) {
        // generate login page
        Log::debug("[LayoutGenerator] Generating Login Page");
        return LoginPageGenerator::GenerateLoginPage();
    }

    Log::debug("[LayoutGenerator] Unknown engagement action, generating error not found page with a go home button.");
    Log::debug("[LayoutGenerator] engagement action: " + std::to_string(action));
    return ErrorPageGenerator::GenerateErrorPage();
}
