#include "LayoutGenerator.h"

#include "moderator_to_vr.pb.h"
#include "layout.pb.h"
#include "rendering_info.pb.h"
#include "./pages/homePage/HomePageGenerator.h"
#include "./pages/homePage/HomePageInfoParser.h"
#include "./pages/debatePage/SingleStatementView/SingleDebatePageGenerator.h"
#include "./pages/debatePage/SingleStatementView/SingleDebatePageInfoParser.h"
#include "./pages/debatePage/FullDebateView/FullDebatePageGenerator.h"
#include "./pages/debatePage/FullDebateView/FullDebatePageInfoParser.h"
#include "./pages/debatePage/StepView/StepView.h"
#include "./pages/debatePage/BriefView/BriefView.h"
#include "./pages/loginPage/LoginPageGenerator.h"
#include "./pages/errorPage/ErrorPageGenerator.h"
#include "./ComponentGenerator.h"
#include "../../utils/Log.h"
#include "../../utils/DemoMode.h"
#include "../../utils/AnonymousUser.h"

#include <cstdlib>
#include <string>

namespace {

// The bar across the top of every page. Left: the tool's name, which goes
// home. Right: "Sign in" for an anonymous visitor; a signed-in user's name
// and a small "Sign out". Built here, once, rather than by each page, so
// every page has the same one.
ui::Component BuildSiteHeader(const user::User& user) {
    ui::Component header = ComponentGenerator::createContainer("siteHeader", "w-full", "", "", "", "", "", "");
    auto& headerCss = *header.mutable_css();
    headerCss["display"] = "flex";
    headerCss["flex-direction"] = "row";
    headerCss["justify-content"] = "space-between";
    headerCss["align-items"] = "center";
    headerCss["flex-wrap"] = "wrap";
    headerCss["gap"] = "0.75rem";
    headerCss["width"] = "100%";
    headerCss["align-self"] = "stretch";
    headerCss["padding-bottom"] = "0.75rem";
    headerCss["border-bottom"] = "1px solid var(--line)";

    // Left: the name, then the home tabs (Home | Explore | Your debates).
    // On the home page the current tab is lit the way the Debate | Brief
    // tabs are; inside a debate none is.
    ui::Component left = ComponentGenerator::createContainer("siteHeaderNav", "", "", "", "", "", "", "");
    (*left.mutable_css())["display"] = "flex";
    (*left.mutable_css())["flex-direction"] = "row";
    (*left.mutable_css())["align-items"] = "center";
    (*left.mutable_css())["flex-wrap"] = "wrap";
    (*left.mutable_css())["gap"] = "0.25rem";

    ui::Component home = ComponentGenerator::createButton(
        "goHomeButton", "Debate Hub", "", "bg-transparent", "", "", "", "", "");
    (*home.mutable_css())["background"] = "transparent";
    (*home.mutable_css())["border"] = "none";
    (*home.mutable_css())["padding"] = "0";
    (*home.mutable_css())["color"] = "var(--text)";
    (*home.mutable_css())["font-size"] = "15px";
    (*home.mutable_css())["font-weight"] = "650";
    (*home.mutable_css())["margin-right"] = "0.75rem";
    ComponentGenerator::addChild(&left, home);

    const bool visitor = anonymous_user::IsAnonymousUsername(user.username());
    const bool onHome = user.engagement().current_action() == user_engagement::ACTION_HOME;
    const std::string active =
        onHome ? HomePageGenerator::ActiveTab(user.engagement().home_view(), visitor) : std::string();
    auto tab = [&](const std::string& id, const std::string& label, bool lit) {
        ComponentGenerator::addChild(&left, StepView::BuildTab(id, label, lit));
    };
    tab("homeTab_home", "Home", active == "home");
    tab("homeTab_explore", "Explore", active == "explore");
    if (!visitor) {
        tab("homeTab_mine", "Your debates", active == "mine");
    }
    ComponentGenerator::addChild(&header, left);

    ui::Component account = ComponentGenerator::createContainer("siteHeaderAccount", "", "", "", "", "", "", "");
    (*account.mutable_css())["display"] = "flex";
    (*account.mutable_css())["flex-direction"] = "row";
    (*account.mutable_css())["align-items"] = "center";
    (*account.mutable_css())["gap"] = "0.75rem";
    if (anonymous_user::IsAnonymousUsername(user.username())) {
        // Outlined, small: the one filled button on a screen is the page's own.
        ComponentGenerator::addChild(&account, ComponentGenerator::createButton(
            "openSignInButton", "Sign in", "", "bg-gray-700", "", "", "", "", "text-sm"));
    } else {
        ComponentGenerator::addChild(&account, ComponentGenerator::createText(
            "headerUsername", user.username(), "text-sm", "text-gray-400", "", ""));
        // An auto-login build has no session to leave.
        if (!demo_mode::autoLogin) {
            ComponentGenerator::addChild(&account, ComponentGenerator::createButton(
                "logoutButton", "Sign out", "logout", "bg-transparent", "", "", "", "", "text-sm"));
        }
    }
    ComponentGenerator::addChild(&header, account);
    return header;
}

// The sign-in form in a modal over whatever page the visitor is on. Its
// title says why it opened: what the visitor pressed (remembered on their
// record, replayed once they sign in), or plain "Sign in" from the header.
ui::Component BuildSignInModal(const user::User& user, const std::string& error,
                               const std::string& prefillUsername) {
    ui::Component overlay = ComponentGenerator::createContainer(
        "signInModalOverlay", "fixed inset-0 flex items-center justify-center", "bg-black/50",
        "", "", "", "", "z-50");
    // Above the docked chat (40) and any page dialog (50).
    (*overlay.mutable_css())["z-index"] = "60";

    ui::Component content = ComponentGenerator::createContainer(
        "signInModalContent", "flex flex-col items-stretch gap-3", "bg-gray-800", "p-8", "",
        "border-2 border-gray-700", "rounded-lg", "w-full max-w-md");

    ComponentGenerator::addChild(&content, ComponentGenerator::createText(
        "signInModalTitle", anonymous_user::SignInTitle(user.pending_action()), "text-xl",
        "text-white", "font-semibold", ""));
    ComponentGenerator::addChild(&content, ComponentGenerator::createText(
        "signInModalDescription",
        "Use a username and password (a new name makes an account), or Google. You will come back to this page.",
        "text-sm", "text-gray-400", "", ""));
    if (!error.empty()) {
        ComponentGenerator::addChild(&content, ComponentGenerator::createText(
            "loginError", error, "text-sm", "text-red-400", "font-medium", ""));
    }

    LoginPageGenerator::AddLoginForm(&content, prefillUsername);

    ComponentGenerator::addChild(&content, ComponentGenerator::createButton(
        "closeSignInButton", "Not now", "", "bg-transparent", "", "", "", "", "text-xs self-end"));

    ComponentGenerator::addChild(&overlay, content);
    return overlay;
}

// Every page's header goes first in its root container; the sign-in modal,
// when it is open, goes last so it draws over everything else.
void AddHeaderAndSignIn(ui::Page& page, const user::User& user, const std::string& signInError,
                        const std::string& signInUsername) {
    if (page.components_size() == 0) {
        return;
    }
    ui::Component* root = page.mutable_components(0);
    *root->add_children() = BuildSiteHeader(user);
    for (int i = root->children_size() - 1; i > 0; --i) {
        root->mutable_children()->SwapElements(i, i - 1);
    }
    if (anonymous_user::IsAnonymousUsername(user.username()) &&
        (user.sign_in_open() || !signInError.empty())) {
        *root->add_children() = BuildSignInModal(user, signInError, signInUsername);
    }
}

ui::Page GeneratePage(const moderator_to_vr::ModeratorToVRMessage& info, VRUserDatabase& userDb);

}  // namespace

ui::Page LayoutGenerator::generateLayout(const moderator_to_vr::ModeratorToVRMessage& info, VRUserDatabase& userDb,
                                         const std::string& signInError, const std::string& signInUsername) {
    ui::Page page = GeneratePage(info, userDb);
    // Every page sets a developer's title bar ("Debate Topic Entry | commit:
    // ..."). It is shown only with DEBATE_DEV_CHROME=1 on the server; the
    // commit is in the home page's footer instead.
    const char* devChrome = std::getenv("DEBATE_DEV_CHROME");
    if (devChrome == nullptr || std::string(devChrome) != "1") {
        page.clear_title();
    }
    if (page.page_id() == "home" || page.page_id() == "debate") {
        AddHeaderAndSignIn(page, info.user(), signInError, signInUsername);
    }
    return page;
}

namespace {
ui::Page GeneratePage(const moderator_to_vr::ModeratorToVRMessage& info, VRUserDatabase& userDb) {
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
            rendering_info::FullDebateViewInfo fullDebateInfo = FullDebatePageInfoParser::ParseFullDebateViewInfo(info.collection(), info.user().user_id(), info.user().username(), info.user().engagement().debating_info().current_claim().id());
            if (info.user().current_scope().has_full_debate() &&
                info.user().current_scope().full_debate().top_view()) {
                if (info.user().current_scope().full_debate().brief_view()) {
                    Log::info("[LayoutGenerator] brief_view=true, generating Brief page");
                    return BriefView::GenerateBriefPage(fullDebateInfo, info.collection(), userDb, info.user());
                }
                Log::info("[LayoutGenerator] top_view=true, generating Step View page");
                return StepView::GenerateStepViewPage(fullDebateInfo, info.collection(), userDb, info.user());
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

    Log::info("[LayoutGenerator] Unknown engagement action, generating error not found page with a go home button.");
    Log::info("[LayoutGenerator] engagement action: " + std::to_string(action));
    return ErrorPageGenerator::GenerateErrorPage();
}
}  // namespace

