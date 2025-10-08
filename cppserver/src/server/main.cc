#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00

#include "../../../src/gen/cpp/page.pb.h"   // ui::Page
#include "../../../src/gen/cpp/event.pb.h"  // debate::UIEvent
#include "../virtualRenderer/pageGenerator.h"
#include "../virtualRenderer/virtualRenderer.h"  // Add this include

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
    renderer.handlePostRequest(req, res);
  });

  // ---------- Start server ----------
  std::cout << "Serving server on http://127.0.0.1:8080\n";
  svr.listen("0.0.0.0", 8080);
}
