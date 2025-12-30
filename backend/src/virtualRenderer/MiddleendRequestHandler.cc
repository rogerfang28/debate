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
#include "./utils/parseCookie.h"

MiddleendRequestHandler::MiddleendRequestHandler() {
    Log::debug("[MiddleendRequestHandler] Initialized.");
}

void MiddleendRequestHandler::handleRequest(const httplib::Request& req, httplib::Response& res) {
    int userId = parseCookie::extractUserIdFromCookies(req);

    // Parse ClientMessage protobuf
    client_message::ClientMessage msg;
    if (!msg.ParseFromString(req.body)) {
      Log::error("Failed to parse ClientMessage");
      res.status = 400;
      res.set_content("Failed to parse ClientMessage", "text/plain");
      return;
    }

    Log::debug("ClientMessage received");
    // log(userId, msg); // detailed logging

    // pass to VR
    // VirtualRenderer renderer;
    ui::Page page = renderer.handleClientMessage(msg, req, res);

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

// make a log function
void MiddleendRequestHandler::log(int userId, const client_message::ClientMessage& msg) {
        // Log the event info
    Log::debug("\n--- Event Info ---");
    Log::debug("Component ID: " + msg.component_id());
    Log::debug("Event Type: " + msg.event_type());
    Log::debug("User ID: " + std::to_string(userId));

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