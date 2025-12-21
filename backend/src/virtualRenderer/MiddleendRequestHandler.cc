// has handle get or post or any request, calls virtual renderer
#include "MiddleendRequestHandler.h"

#include "../../../src/gen/cpp/client_message.pb.h"
#include "../utils/pathUtils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

#include "../../../src/gen/cpp/layout.pb.h"
#include "./virtualRenderer.h"
#include <iostream>
#include "./LayoutGenerator/pages/loginPage/LoginPageGenerator.h"


void MiddleendRequestHandler::handleRequest(const httplib::Request& req, httplib::Response& res) {
    std::string user = extractUserFromCookies(req);

    // Parse ClientMessage protobuf
    client_message::ClientMessage msg;
    if (!msg.ParseFromString(req.body)) {
      std::cerr << "Failed to parse ClientMessage\n";
      res.status = 400;
      res.set_content("Failed to parse ClientMessage", "text/plain");
      return;
    }

    std::cout << "ClientMessage received\n";
    
    // Log the event info
    std::cout << "\n--- Event Info ---\n";
    std::cout << "Component ID: " << msg.component_id() << "\n";
    std::cout << "Event Type: " << msg.event_type() << "\n";
    std::cout << "User: " << user << "\n";

    // Log page data
    if (msg.has_page_data() && true) { // set to true to enable detailed logging
      const auto& page_data = msg.page_data();
      std::cout << "\n--- Page Data ---\n";
      std::cout << "Page ID: " << page_data.page_id() << "\n";
      std::cout << "Components count: " << page_data.components_size() << "\n";
      
    //   std::cout << "\n--- All Components ---\n";
    //   for (int i = 0; i < page_data.components_size(); i++) {
    //     const auto& comp = page_data.components(i);
    //     std::cout << "  [" << i << "] id: \"" << comp.id() 
    //               << "\" = \"" << comp.value() << "\"\n";
    //   }
    } else {
      std::cout << "!!! No page data included\n";
    }

    const std::string& componentId = msg.component_id();
    const std::string& eventType = msg.event_type();
    const std::string& pageId = msg.page_data().page_id();
    
    // hard code the username part for now because idk how to do it properly
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
                std::cout << "[Auth] Setting user cookie: " << username << "\n";
                res.set_header("Set-Cookie", "user=" + username + "; Path=/; Max-Age=2592000; SameSite=Lax");
            } else {
                std::cerr << "[Auth] Invalid username: " << username << "\n";
                // return login page again
                ui::Page loginPage = createLoginPage();
                std::string loginPageSerialized;
                if (!loginPage.SerializeToString(&loginPageSerialized)) {
                    std::cerr << "[Auth] Failed to serialize login page\n";
                    res.status = 500;
                    res.set_content("Failed to serialize login page", "text/plain");
                    return;
                }
                res.set_content(loginPageSerialized, "application/x-protobuf");
                res.status = 200;
                return;
            }
        }
    }

    // logout handling
    // std::cout << (componentId == "logoutButton" && eventType == "onClick") << "\n";
    // std::cout << componentId << " " << eventType << "\n";
    if (componentId == "logoutButton" && eventType == "onClick") {
        // ============ SIGN-OUT ============
        std::cout << "[Auth] Signing out user: " << user << "\n";
        res.set_header("Set-Cookie", "user=guest; Path=/; Max-Age=0; SameSite=Lax");
        res.status = 200;
        // create login page
        ui::Page loginPage = createLoginPage();
        std::string loginPageSerialized;
        if (!loginPage.SerializeToString(&loginPageSerialized)) {
            std::cerr << "[Auth] Failed to serialize login page\n";
            res.status = 500;
            res.set_content("Failed to serialize login page", "text/plain");
            return;
        }
        res.set_content(loginPageSerialized, "application/x-protobuf");
        return;
    }

    if (user == "guest") {
        // if still guest, return login page
        ui::Page loginPage = createLoginPage();
        std::string loginPageSerialized;
        if (!loginPage.SerializeToString(&loginPageSerialized)) {
            std::cerr << "[Auth] Failed to serialize login page\n";
            res.status = 500;
            res.set_content("Failed to serialize login page", "text/plain");
            return;
        }
        res.set_content(loginPageSerialized, "application/x-protobuf");
        res.status = 200;
        return;
    }

    // pass to VR
    VirtualRenderer renderer;
    ui::Page page = renderer.handleClientMessage(msg, user);

    // send proto back in the res
    std::string page_data_serialized;
    if (!page.SerializeToString(&page_data_serialized)) {
        std::cerr << "❌ Failed to serialize Page response\n";
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
        std::cout << "[Auth] No cookie found, returning 'guest'\n";
        return "guest";
    }

    // Parse cookies looking for "user=<username>"
    std::string prefix = "user=";
    size_t pos = cookie_header.find(prefix);
    if (pos == std::string::npos) {
        std::cout << "[Auth] No 'user=' cookie found, returning 'guest'\n";
        return "guest";
    }

    size_t start = pos + prefix.length();
    size_t end = cookie_header.find(';', start);
    std::string user = (end == std::string::npos) 
        ? cookie_header.substr(start)
        : cookie_header.substr(start, end - start);

    std::cout << "[Auth] Extracted user from cookie: '" << user << "'\n";
    return user;
}

ui::Page MiddleendRequestHandler::createLoginPage() {
    // use LoginPageGenerator to create login page
    ui::Page loginPage = LoginPageGenerator::GenerateLoginPage();
    return loginPage;
}