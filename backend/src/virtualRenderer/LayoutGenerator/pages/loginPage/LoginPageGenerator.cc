#include "LoginPageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../utils/DemoMode.h"
#include "../../../../utils/Log.h"

ui::Page LoginPageGenerator::GenerateLoginPage(const std::string& errorMessage,
                                               const std::string& prefillUsername) {
    ui::Page page;
    page.set_page_id("login");
    page.set_title((demo_mode::autoLogin ? "Please Reload" : (demo_mode::kDemoEnabled ? "Demo Login" : "Sign In")) + std::string(" | commit: ") + GIT_COMMIT_HASH);

    // Main container
    ui::Component main = GenerateLoginPageMainLayout(errorMessage, prefillUsername);

    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(main);
    return page;
}









ui::Component LoginPageGenerator::GenerateLoginPageMainLayout(const std::string& errorMessage,
                                                              const std::string& prefillUsername) {
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
        demo_mode::autoLogin ? "Please Reload" : (demo_mode::kDemoEnabled ? "Click Demo to continue." : "Log in with a username and password, sign in with Google, or continue as guest."),
        "",
        "text-gray-300",
        "",
        "text-center"
    );
    ComponentGenerator::addChild(&main, description);

    // Error message (e.g. wrong password) — only shown when present.
    if (!errorMessage.empty()) {
        ui::Component error = ComponentGenerator::createText(
            "loginError",
            errorMessage,
            "",
            "text-red-400",
            "font-medium",
            "text-center"
        );
        ComponentGenerator::addChild(&main, error);
    }

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
              "w-full max-w-sm",
              prefillUsername
          );
          ComponentGenerator::addChild(&main, usernameInput);

          // Password input (masked via type="password")
          ui::Component passwordInput = ComponentGenerator::createInput(
              "passwordInput",
              "Enter your password...",
              "password",
              "bg-gray-800",
              "text-white",
              "border-gray-600",
              "p-3",
              "rounded",
              "w-full max-w-sm"
          );
          ComponentGenerator::addAttribute(&passwordInput, "type", "password");
          ComponentGenerator::addChild(&main, passwordInput);

          // Log In button (username + password)
          ui::Component submitButton = ComponentGenerator::createButton(
              "submitButton",
              "Log In",
              "submit",
              "bg-green-600",
              "hover:bg-green-500",
              "text-white",
              "px-6 py-3",
              "rounded-lg",
              "transition"
          );
          ComponentGenerator::addChild(&main, submitButton);

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