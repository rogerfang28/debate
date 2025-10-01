#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00

#include "../../../src/gen/cpp/page.pb.h"   // ui::Page
#include "../../../src/gen/cpp/event.pb.h"  // debate::UIEvent
#include "../virtualRenderer/pageGenerator.h"
#include "../main/eventHandler.h"

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
  svr.Get("/", [&](const httplib::Request& req, httplib::Response& res) {
    std::string user = "defaultUser"; // later: parse from req.query_string
    std::string page_bin = generatePage(user, exe_dir());

    if (page_bin.empty()) {
      res.status = 500;
      res.set_content("Failed to generate page\n", "text/plain");
      return;
    }

    res.set_content(page_bin, "application/x-protobuf");
    std::cout << "GET / served generated page for user=" << user
              << " (" << page_bin.size() << " bytes)\n";
  });

  // ---------- POST / ----------
  svr.Post("/", [&](const httplib::Request& req, httplib::Response& res) {
    if (req.get_header_value("Content-Type") != "application/x-protobuf") {
      res.status = 415;
      res.set_content("Unsupported Media Type: expected application/x-protobuf\n", "text/plain");
      return;
    }

    debate::UIEvent evt;
    if (!evt.ParseFromArray(req.body.data(), (int)req.body.size())) {
      res.status = 400;
      res.set_content("Bad Request: failed to parse debate.UIEvent\n", "text/plain");
      return;
    }

    // Handle with EventHandler
    EventHandler handler(std::move(evt));
    handler.handleEvent("defaultUser");

    res.status = 204; // No Content
    std::cout << "POST / handled debate.UIEvent\n";
  });

  // ---------- Start server ----------
  std::cout << "Serving on http://127.0.0.1:8080\n";
  svr.listen("0.0.0.0", 8080);
}
