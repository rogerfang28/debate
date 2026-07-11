#include "LoginPageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../utils/DemoMode.h"
#include "../../../../utils/Log.h"

ui::Page LoginPageGenerator::GenerateLoginPage() {
    ui::Page page;
    page.set_page_id("login");
    page.set_title((demo_mode::autoLogin ? "Please Reload" : (demo_mode::kDemoEnabled ? "Demo Login" : "Sign In")) + std::string(" | commit: ") + GIT_COMMIT_HASH);

    // Main container
    ui::Component main = GenerateLoginPageMainLayout();

    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(main);
    return page;
}









ui::Component LoginPageGenerator::GenerateLoginPageMainLayout() {
    // Main container
    ui::Component main = ComponentGenerator::createContainer(
        "main",
        "min-h-screen flex flex-col items-center justify-center gap-6",
        "bg-gray-900",
        "p-8",
        "",
        "",
        "",
        "text-white"
    );

    // Title
    ui::Component title = ComponentGenerator::createText(
        "title",
        demo_mode::autoLogin ? "Please Reload" : "Welcome!",
        "text-3xl",
        "text-blue-400",
        "font-bold",
        ""
    );
    ComponentGenerator::addChild(&main, title);

    // Description
    ui::Component description = ComponentGenerator::createText(
        "description",
        demo_mode::autoLogin ? "Please Reload" : (demo_mode::kDemoEnabled ? "Click Demo to continue." : "Sign in with Google, or continue as guest."),
        "",
        "text-gray-300",
        "",
        "text-center"
    );
    ComponentGenerator::addChild(&main, description);

    if (demo_mode::autoLogin) {
        return main;
    }

    if (demo_mode::kDemoEnabled) {
        ui::Component demoButton = ComponentGenerator::createButton(
            "demoButton",
            "Demo",
            "demo",
            "bg-blue-600",
            "hover:bg-blue-500",
            "text-white",
            "px-6 py-3",
            "rounded-lg",
            "transition"
        );
        ComponentGenerator::addChild(&main, demoButton);
    }
    else {
          // Guest Mode button — signs in as username "guest"
          ui::Component guestButton = ComponentGenerator::createButton(
              "guestButton",
              "Continue as Guest",
              "guest",
              "bg-gray-600",
              "hover:bg-gray-500",
              "text-white",
              "px-6 py-3",
              "rounded-lg",
              "transition"
          );
          ComponentGenerator::addChild(&main, guestButton);

          // Google Sign-In button (only show if GOOGLE_CLIENT_ID is set)
            if (std::getenv("GOOGLE_CLIENT_ID") != nullptr) {
                Log::info("[LoginPageGenerator] GOOGLE_CLIENT_ID is set, including googleLoginButton");
                ui::Component googleButton = ComponentGenerator::createContainer(
                    "googleLoginButton",
                    "google-login"
                );
                ComponentGenerator::addChild(&main, googleButton);
            } else {
                Log::warn("[LoginPageGenerator] GOOGLE_CLIENT_ID is NOT set — googleLoginButton will NOT be included");
            }
        }

    return main;
}