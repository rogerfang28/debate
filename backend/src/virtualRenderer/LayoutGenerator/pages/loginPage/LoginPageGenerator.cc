#include "LoginPageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../utils/DemoMode.h"

ui::Page LoginPageGenerator::GenerateLoginPage() {
    ui::Page page;
    page.set_page_id("login");
    page.set_title((demo_mode::autoLogin ? "Please Reload" : (demo_mode::kDemoEnabled ? "Demo Login" : "Enter Username")) + std::string(" | commit: ") + GIT_COMMIT_HASH);

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
        demo_mode::autoLogin ? "Please Reload" : (demo_mode::kDemoEnabled ? "Click Demo to continue." : "Please enter your username to continue."),
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
        // Username input
        ui::Component usernameInput = ComponentGenerator::createInput(
            "usernameInput",
            "Enter your username...",
            "username",
            "bg-gray-800",
            "text-white",
            "border-gray-600",
            "p-3",
            "rounded",
            "w-full max-w-sm"
        );
        ComponentGenerator::addChild(&main, usernameInput);

        // Submit button
          ui::Component submitButton = ComponentGenerator::createButton(
              "submitButton",
              "Submit",
              "submit",
              "bg-green-600",
              "hover:bg-green-500",
              "text-white",
              "px-6 py-3",
              "rounded-lg",
              "transition"
          );
          ComponentGenerator::addChild(&main, submitButton);

          // Google Sign-In button (only show if GOOGLE_CLIENT_ID is set)
           if (std::getenv("GOOGLE_CLIENT_ID") != nullptr) {
               ui::Component googleButton = ComponentGenerator::createContainer(
                   "googleLoginButton",
                   "google-login"
               );
               ComponentGenerator::addChild(&main, googleButton);
           }
        }

    return main;
}