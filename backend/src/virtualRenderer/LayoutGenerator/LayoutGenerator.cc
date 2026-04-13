#include "LayoutGenerator.h"

#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/rendering_info.pb.h"
#include "./pages/homePage/HomePageGenerator.h"
#include "./pages/homePage/HomePageInfoParser.h"
#include "./pages/debatePage/SingleStatementView/SingleDebatePageGenerator.h"
#include "./pages/debatePage/SingleStatementView/SingleDebatePageInfoParser.h"
#include "./pages/debatePage/FullDebateView/FullDebatePageGenerator.h"
#include "./pages/debatePage/FullDebateView/FullDebatePageInfoParser.h"
#include "./pages/debatePage/StepView/StepView.h"
#include "./pages/loginPage/LoginPageGenerator.h"
#include "./pages/errorPage/ErrorPageGenerator.h"
#include "../../utils/Log.h"

ui::Page LayoutGenerator::generateLayout(const moderator_to_vr::ModeratorToVRMessage& info, VRUserDatabase& userDb) {
    // ok so we need to decode the info and generate a page based on it
    // so we should check user engagement first
    const auto action = info.user().engagement().current_action();
    Log::debug("[LayoutGenerator] Collection - " + std::to_string(info.collection().claims_by_id_size()) + " claims, " + std::to_string(info.collection().links_by_id_size()) + " links");
    if (action == user_engagement::ACTION_HOME) {
        // generate home page
        Log::info("[LayoutGenerator] Generating Home Page");
        rendering_info::HomePageRenderingInfo homePageInfo = HomePageInfoParser::ParseFromResponse(info);
        return HomePageGenerator::GenerateHomePage(homePageInfo);
    }

    if (action == user_engagement::ACTION_DEBATING) {
        // generate debate page
        Log::info("[LayoutGenerator] Generating Debate Page");
        debate::ScopeType scopeType = info.user().current_scope().scopetype();
        if (scopeType == debate::SINGLE_CLAIM) {
            Log::info("[LayoutGenerator] Generating Single Claim Debate Page");
            rendering_info::DebatePageRenderingInfo debatePageInfo = DebatePageInfoParser::ParseFromUser(info.user(), info.collection());
            return DebatePageGenerator::GenerateSingleDebatePage(debatePageInfo, info.user());
        } else if (scopeType == debate::FULL_DEBATE) {
            Log::info("[LayoutGenerator] Generating Full Debate Page");
            rendering_info::FullDebateViewInfo fullDebateInfo = FullDebatePageInfoParser::ParseFullDebateViewInfo(info.collection());
            if (info.user().current_scope().has_full_debate() &&
                info.user().current_scope().full_debate().top_view()) {
                Log::info("[LayoutGenerator] top_view=true, generating Step View page");
                return StepView::GenerateStepViewPage(fullDebateInfo, info.collection(), userDb);
            }
            rendering_info::DebatePageRenderingInfo debatePageInfo = FullDebatePageInfoParser::ParseFromUser(info.user(), info.collection());
            return FullDebatePageGenerator::GenerateFullDebatePage(debatePageInfo, fullDebateInfo, info.user());
        }
        else {
            Log::warn("[LayoutGenerator] Unknown scope type, defaulting to generating Single Claim Debate Page");
            rendering_info::DebatePageRenderingInfo debatePageInfo = DebatePageInfoParser::ParseFromUser(info.user(), info.collection());
            return DebatePageGenerator::GenerateSingleDebatePage(debatePageInfo, info.user());
        }
    }

    if (action == user_engagement::ACTION_LOGIN) {
        // generate login page
        Log::info("[LayoutGenerator] Generating Login Page");
        return LoginPageGenerator::GenerateLoginPage();
    }

    Log::info("[LayoutGenerator] Unknown engagement action, generating error not found page with a go home button.");
    Log::info("[LayoutGenerator] engagement action: " + std::to_string(action));
    return ErrorPageGenerator::GenerateErrorPage();
}
