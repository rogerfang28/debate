#include "LoginPageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../utils/Log.h"

#include <cstdlib>

// The sign-in modal's form: username, password, the sign-in button, Google's
// button, and "continue as guest" as a quiet link under the rest, for someone
// who wants to contribute without an account. The submit button is
// signInSubmitButton because the home page already has a "submitButton" (its
// topic box).
void LoginPageGenerator::AddLoginForm(ui::Component* parent, const std::string& prefillUsername) {
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
    ComponentGenerator::addChild(parent, usernameInput);

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
    ComponentGenerator::addChild(parent, passwordInput);

    // Sign in button (username + password)
    ui::Component submitButton = ComponentGenerator::createButton(
        "signInSubmitButton",
        "Sign in",
        "submit",
        "bg-green-600",
        "hover:bg-green-500",
        "text-white",
        "px-6 py-3",
        "rounded-lg",
        "transition w-full max-w-sm"
    );
    ComponentGenerator::addChild(parent, submitButton);

    // Google Sign-In button (only shown if GOOGLE_CLIENT_ID is set). The id is
    // what the client keys Google's button on.
    if (std::getenv("GOOGLE_CLIENT_ID") != nullptr) {
        Log::info("[LoginPageGenerator] GOOGLE_CLIENT_ID is set, including googleLoginButton");
        ui::Component googleButton = ComponentGenerator::createContainer(
            "googleLoginButton",
            "google-login"
        );
        ComponentGenerator::addChild(parent, googleButton);
    } else {
        Log::warn("[LoginPageGenerator] GOOGLE_CLIENT_ID is NOT set — googleLoginButton will NOT be included");
    }

    // "Continue as guest" -- signs in as the shared, writable "guest".
    ComponentGenerator::addChild(parent, ComponentGenerator::createButton(
        "guestButton", "Continue as guest", "guest", "bg-transparent",
        "hover:text-gray-200", "text-gray-400", "px-2 py-1", "rounded", "text-xs underline"));
}
