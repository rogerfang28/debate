#pragma once

#include <string>
#include "../../../src/gen/cpp/layout.pb.h"

// Forward declarations for UI component manipulation
namespace ui {
    class Component;
}

// Helper functions for component manipulation
ui::Component* findComponent(ui::Component* root, const std::string& id);
void updateComponentText(ui::Component* root, const std::string& id, const std::string& newText);

// ---------------------------------------------------------
// Function: generatePage
// ---------------------------------------------------------
// Generates a UI page (as a serialized protobuf string)
// customized for a specific user.
//
// The function:
//  - Loads a .pbtxt UI schema (e.g. testPage.pbtxt)
//  - Fetches the user's debates from the database via
//    DebateDatabaseHandler
//  - Injects those topics into the "topicsList" component
//  - Returns the serialized protobuf page in binary form
//
// Returns: Serialized page data (application/x-protobuf)
//
// Example usage:
//   std::string page = generatePage("adam23");
//   // send 'page' as HTTP response
// ---------------------------------------------------------
std::string generateHomePage(const std::string& user);
std::string generateDebateClaimPage(const std::string& user, const std::string& topic, const std::string& curClaim);
std::string generatePage(const std::string& user);
std::string generateSignInPage();
