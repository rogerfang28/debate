// has handle get or post or any request, calls virtual renderer
#include "MiddleendRequestHandler.h"

// #include "../../../src/gen/cpp/client_message.pb.h"
#include "../utils/pathUtils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

#include "../../../src/gen/cpp/layout.pb.h"
#include "./virtualRenderer.h"
#include <iostream>
#include "./LayoutGenerator/pages/loginPage/LoginPageGenerator.h"
#include "../utils/Log.h"


void MiddleendRequestHandler::handleRequest(const httplib::Request& req, httplib::Response& res) {
    std::string user = extractUserFromCookies(req);

    // Parse ClientMessage protobuf
    client_message::ClientMessage msg;
    if (!msg.ParseFromString(req.body)) {
      Log::error("Failed to parse ClientMessage");
      res.status = 400;
      res.set_content("Failed to parse ClientMessage", "text/plain");
      return;
    }

    Log::debug("ClientMessage received");

    // log(user, msg); // detailed logging

    if (!validateAuth(msg, res, user)) {
        return; // auth failed and response already set
    }

    // pass to VR
    VirtualRenderer renderer;
    ui::Page page = renderer.handleClientMessage(msg, user);

    // send proto back in the res
    std::string page_data_serialized;
    if (!page.SerializeToString(&page_data_serialized)) {
        Log::error("❌ Failed to serialize Page response");
        res.status = 500;
        res.set_content("Failed to serialize Page response", "text/plain");
        return;
    }
    res.status = 200;
    res.set_content(page_data_serialized, "application/x-protobuf");
}

// Helper to extract user from cookies
std::string MiddleendRequestHandler::extractUserFromCookies(const httplib::Request& req) {
    auto cookie_header = req.get_header_value("Cookie");
    // std::cout << "[Auth] Cookie header: '" << cookie_header << "'\n";
    
    if (cookie_header.empty()) {
        Log::debug("[Auth] No cookie found, returning 'guest'");
        return "guest";
    }

    // Parse cookies looking for "user=<username>"
    std::string prefix = "user=";
    size_t pos = cookie_header.find(prefix);
    if (pos == std::string::npos) {
        Log::debug("[Auth] No 'user=' cookie found, returning 'guest'");
        return "guest";
    }

    size_t start = pos + prefix.length();
    size_t end = cookie_header.find(';', start);
    std::string user = (end == std::string::npos) 
        ? cookie_header.substr(start)
        : cookie_header.substr(start, end - start);

    Log::debug("[Auth] Extracted user from cookie: '" + user + "'");
    return user;
}

// make a log function
void MiddleendRequestHandler::log(const std::string& user, const client_message::ClientMessage& msg) {
        // Log the event info
    Log::debug("\n--- Event Info ---");
    Log::debug("Component ID: " + msg.component_id());
    Log::debug("Event Type: " + msg.event_type());
    Log::debug("User: " + user);

    // Log page data
    if (msg.has_page_data() && true) { // set to true to enable detailed logging
      const auto& page_data = msg.page_data();
      Log::debug("\n--- Page Data ---");
      Log::debug("Page ID: " + page_data.page_id());
      Log::debug("Components count: " + std::to_string(page_data.components_size()));
      
      Log::debug("\n--- All Components ---");
      for (int i = 0; i < page_data.components_size(); i++) {
        const auto& comp = page_data.components(i);
        Log::debug("  [" + std::to_string(i) + "] id: \"" + comp.id() 
                  + "\" = \"" + comp.value() + "\"");
      }
    } else {
      Log::debug("!!! No page data included");
    }
}

bool MiddleendRequestHandler::validateAuth(client_message::ClientMessage& msg, httplib::Response& res, const std::string& user) {
    const std::string& componentId = msg.component_id();
    const std::string& eventType = msg.event_type();
    const std::string& pageId = msg.page_data().page_id();
    
    // hard code the username part for now because idk how to do it properly
    // function to check if user is logged in
    if (pageId == "enter_username") {
        // ============ SIGN-IN PAGE ============
        if (componentId == "submitButton" && eventType == "onClick") {
            // Extract username from the page data
            std::string username;
            for (const auto& comp : msg.page_data().components()) {
                if (comp.id() == "usernameInput") {
                    username = comp.value();
                    break;
                }
            }
            
            if (!username.empty() && username != "guest") {
                // Set cookie and redirect by setting a special response
                Log::debug("[Auth] Setting user cookie: " + username);
                res.set_header("Set-Cookie", "user=" + username + "; Path=/; Max-Age=2592000; SameSite=Lax");
            } else {
                Log::error("[Auth] Invalid username: " + username);
                // return login page again
                ui::Page loginPage = LoginPageGenerator::GenerateLoginPage();
                std::string loginPageSerialized;
                if (!loginPage.SerializeToString(&loginPageSerialized)) {
                    Log::error("[Auth] Failed to serialize login page");
                    res.status = 500;
                    res.set_content("Failed to serialize login page", "text/plain");
                    return false;
                }
                res.set_content(loginPageSerialized, "application/x-protobuf");
                res.status = 200;
                return false;
            }
        }
    }

    // logout handling
    // std::cout << (componentId == "logoutButton" && eventType == "onClick") << "\n";
    // std::cout << componentId << " " << eventType << "\n";
    if (componentId == "logoutButton" && eventType == "onClick") {
        // ============ SIGN-OUT ============
        Log::debug("[Auth] Signing out user: " + user);
        res.set_header("Set-Cookie", "user=guest; Path=/; Max-Age=0; SameSite=Lax");
        res.status = 200;
        // create login page
        ui::Page loginPage = LoginPageGenerator::GenerateLoginPage();
        std::string loginPageSerialized;
        if (!loginPage.SerializeToString(&loginPageSerialized)) {
            Log::error("[Auth] Failed to serialize login page");
            res.status = 500;
            res.set_content("Failed to serialize login page", "text/plain");
            return false;
        }
        res.set_content(loginPageSerialized, "application/x-protobuf");
        return false;
    }

    if (user == "guest") {
        // if still guest, return login page
        ui::Page loginPage = LoginPageGenerator::GenerateLoginPage();
        std::string loginPageSerialized;
        if (!loginPage.SerializeToString(&loginPageSerialized)) {
            Log::error("[Auth] Failed to serialize login page");
            res.status = 500;
            res.set_content("Failed to serialize login page", "text/plain");
            return false;
        }
        res.set_content(loginPageSerialized, "application/x-protobuf");
        res.status = 200;
        return false;
    }
    return true;
}