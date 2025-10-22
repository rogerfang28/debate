#include "virtualRenderer.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/event.pb.h"
#include "../../../src/gen/cpp/client_message.pb.h"
#include "./pageGenerator.h"
#include "../debate/main/EventHandler.h"
#include "../server/httplib.h"
#include <iostream>

// Constructor
VirtualRenderer::VirtualRenderer() {
    std::cout << "VirtualRenderer initialized.\n";
}

// Destructor
VirtualRenderer::~VirtualRenderer() {
    std::cout << "VirtualRenderer destroyed.\n"; // probably never called
}

// Helper to extract user from cookies
std::string VirtualRenderer::extractUserFromCookies(const httplib::Request& req) {
    auto cookie_header = req.get_header_value("Cookie");
    std::cout << "[Auth] Cookie header: '" << cookie_header << "'\n";
    
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

// Handle GET requests
void VirtualRenderer::handleGetRequest(const httplib::Request& req, httplib::Response& res) {
    std::string user = extractUserFromCookies(req);
    std::cout << "[GET] User determined: '" << user << "'\n";
    
    // If no user cookie, show sign-in page
    if (user == "guest") {
        std::cout << "[GET] Serving sign-in page...\n";
        std::string page_bin = generateSignInPage();
        std::cout << "[GET] Sign-in page size: " << page_bin.size() << " bytes\n";
        res.set_content(page_bin, "application/x-protobuf");
        std::cout << "[GET] ✅ Served sign-in page (no user cookie)\n";
        return;
    }
    
    std::cout << "[GET] Serving authenticated page for user: " << user << "\n";
    std::string page_bin = generatePage(user);
    res.set_content(page_bin, "application/x-protobuf");
    std::cout << "[GET] ✅ Served page for user=" << user
            << " (" << page_bin.size() << " bytes)\n";
}

// // Handle POST requests
// void VirtualRenderer::handlePostRequest(const httplib::Request& req, httplib::Response& res) {
//     if (req.get_header_value("Content-Type") != "application/x-protobuf") {
//         res.status = 415;
//         res.set_content("Unsupported Media Type: expected application/x-protobuf\n", "text/plain");
//         return;
//     }

//     debate::UIEvent evt;
//     if (!evt.ParseFromArray(req.body.data(), (int)req.body.size())) {
//         res.status = 400;
//         res.set_content("Bad Request: failed to parse debate.UIEvent\n", "text/plain");
//         return;
//     }

//     // // Handle with EventHandler
//     EventHandler handler(std::move(evt));
//     handler.handleEvent("defaultUser");

//     res.status = 204; // No Content
//     std::cout << "POST / handled debate.UIEvent\n";
// }

void VirtualRenderer::handleClientMessage(const httplib::Request& req, httplib::Response& res) {
    std::string user = extractUserFromCookies(req);
    // Parse ClientMessage protobuf
    clientmessage::ClientMessage client_message;
    if (!client_message.ParseFromString(req.body)) {
      std::cerr << "❌ Failed to parse ClientMessage\n";
      res.status = 400;
      res.set_content("Failed to parse ClientMessage", "text/plain");
      return;
    }

    std::cout << "\n========================================\n";
    std::cout << "📬 ClientMessage received\n";
    std::cout << "✅ ClientMessage parsed successfully!\n";
    
    // Log the event info
    std::cout << "\n--- Event Info ---\n";
    std::cout << "Component ID: " << client_message.component_id() << "\n";
    std::cout << "Event Type: " << client_message.event_type() << "\n";
    std::cout << "User: " << user << "\n";

    // Log page data
    if (client_message.has_page_data()) {
      const auto& page_data = client_message.page_data();
      std::cout << "\n--- Page Data ---\n";
      std::cout << "Page ID: " << page_data.page_id() << "\n";
      std::cout << "Components count: " << page_data.components_size() << "\n";
      
      std::cout << "\n--- All Components ---\n";
      for (int i = 0; i < page_data.components_size(); i++) {
        const auto& comp = page_data.components(i);
        std::cout << "  [" << i << "] id: \"" << comp.id() 
                  << "\" = \"" << comp.value() << "\"\n";
      }
    } else {
      std::cout << "⚠️ No page data included\n";
    }

    // Translate ClientMessage to Event protobuf
    event::Event evt;
    
    const std::string& componentId = client_message.component_id();
    const std::string& eventType = client_message.event_type();
    const std::string& pageId = client_message.page_data().page_id();
    
    std::cout << "\n--- Translating to Event ---\n";
    std::cout << "  Page ID: " << pageId << "\n";
    
    // Route based on page_id first, then component_id + event_type
    if (pageId == "enter_username") {
        // ============ SIGN-IN PAGE ============
        if (componentId == "submitButton" && eventType == "onClick") {
            // Extract username from the page data
            std::string username;
            for (const auto& comp : client_message.page_data().components()) {
                if (comp.id() == "usernameInput") {
                    username = comp.value();
                    break;
                }
            }
            
            if (!username.empty() && username != "guest") {
                // Set cookie and redirect by setting a special response
                std::cout << "[Auth] Setting user cookie: " << username << "\n";
                res.set_header("Set-Cookie", "user=" + username + "; Path=/; Max-Age=2592000; SameSite=Lax");
                res.status = 200;
                res.set_content("OK", "text/plain");
                return;
            } else {
                std::cerr << "[Auth] Invalid username: " << username << "\n";
                res.status = 400;
                res.set_content("Invalid username", "text/plain");
                return;
            }
        }
    } else if (pageId == "home") {
        // ============ HOME PAGE ============
        if (componentId == "submitButton" && eventType == "onClick") {
            // Submit topic action
            evt.set_action_id("submitTopic");
            // Extract topicInput value from page_data
            for (const auto& comp : client_message.page_data().components()) {
                if (comp.id() == "topicInput") {
                    (*evt.mutable_data())["topicInput"] = comp.value();
                    std::cout << "  Added data: topicInput = " << comp.value() << "\n";
                    break;
                }
            }
        } else if (componentId == "clearButton" && eventType == "onClick") {
            evt.set_action_id("clearTopics");
        } else if (componentId == "logoutButton" && eventType == "onClick") {
            // Logout - clear cookie and return success
            std::cout << "[Auth] Logging out user: " << user << "\n";
            res.set_header("Set-Cookie", "user=; Path=/; Max-Age=0");
            res.status = 200;
            res.set_content("Logged out", "text/plain");
            return;
        } else if (componentId.find("topicButton_") == 0 && eventType == "onClick") {
            evt.set_action_id("enterTopic");
            std::string topicID = componentId.substr(12); // Extract ID after "topicButton_"
            (*evt.mutable_data())["topicID"] = topicID;
            std::cout << "  Added data: topicID = " << topicID << "\n";
        } else {
            std::cerr << "❌ Unknown component/event combination on home page: " 
                      << componentId << "/" << eventType << "\n";
            res.status = 400;
            res.set_content("Unknown event", "text/plain");
            return;
        }
    } else {
        // ============ OTHER PAGES ============
        if (componentId == "goHomeButton" && eventType == "onClick") {
            evt.set_action_id("goHome");
        } else {
            std::cerr << "❌ Unknown page or event: page=" << pageId 
                      << " component=" << componentId << "/" << eventType << "\n";
            res.status = 400;
            res.set_content("Unknown event", "text/plain");
            return;
        }
    }

    std::cout << "  Action ID: " << evt.action_id() << "\n";
    std::cout << "========================================\n\n";

    // Pass translated Event to EventHandler
    EventHandler handler;
    handler.handleEvent(evt, user);
    res.status = 204;  // No Content
}