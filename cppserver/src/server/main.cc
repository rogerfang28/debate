#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00

#include "../virtualRenderer/virtualRenderer.h"  // Add this include
#include "../../../src/gen/cpp/client_message.pb.h"

#include "httplib.h"
#include <google/protobuf/text_format.h>
#include <windows.h>
#include <filesystem>
#include <iostream>

// ---------- util: exe directory ----------
static std::filesystem::path exe_dir() {
  wchar_t buf[MAX_PATH];
  DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
  return std::filesystem::path(std::wstring(buf, n)).parent_path();
}

int main() {
  httplib::Server svr;
  VirtualRenderer renderer;  // Create the renderer instance

  // ---------- CORS middleware ----------
  svr.set_pre_routing_handler([](const httplib::Request &req, httplib::Response &res) {
    res.set_header("Access-Control-Allow-Origin", "*");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "content-type");
    res.set_header("Access-Control-Max-Age", "86400");
    if (req.method == "OPTIONS") {
      res.status = 204;
      return httplib::Server::HandlerResponse::Handled;
    }
    return httplib::Server::HandlerResponse::Unhandled;
  });

  // ---------- GET / ----------
  svr.Get("/", [&renderer](const httplib::Request& req, httplib::Response& res) {
    renderer.handleGetRequest(req, res);
  });

  // ---------- POST / ----------
  svr.Post("/", [&renderer](const httplib::Request& req, httplib::Response& res) {
    // renderer.handlePostRequest(req, res);
    std::cout << "testAAAAAAAAAAAAAAAAA";
  });

  svr.Post("/clientmessage", [&renderer](const httplib::Request& req, httplib::Response& res) {
    std::cout << "CLIENT MESSAGE RECIEVED";
    // renderer.handlePostRequest(req, res);
    std::cout << "\n========================================\n";
    std::cout << "📬 POST / received\n";
    
    // Parse ClientMessage protobuf
    debate::ClientMessage client_message;
    if (!client_message.ParseFromString(req.body)) {
      std::cerr << "❌ Failed to parse ClientMessage\n";
      res.status = 400;
      res.set_content("Failed to parse ClientMessage", "text/plain");
      return;
    }

    // Log the event info
    std::cout << "✅ ClientMessage parsed successfully!\n";
    std::cout << "\n--- Event Info ---\n";
    std::cout << "Component ID: " << client_message.component_id() << "\n";
    std::cout << "Event Type: " << client_message.event_type() << "\n";

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
    
    std::cout << "========================================\n\n";

    res.status = 204;  // No Content
  });

  // ---------- Start server ----------
  std::cout << "Serving server on http://127.0.0.1:8080\n";
  svr.listen("0.0.0.0", 8080);
}
