#include "MiddleendRequestHandler.h"
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
  // VirtualRenderer renderer;  // Create the renderer instance
  MiddleendRequestHandler handler;

  // ---------- CORS middleware ----------
  svr.set_pre_routing_handler([](const httplib::Request &req, httplib::Response &res) {
    // Allow credentials (cookies) - must specify exact origin, not "*"
    std::string origin = req.get_header_value("Origin");
    if (origin.empty()) {
      origin = "http://localhost:5173"; // Default for Vite dev server
    }
    res.set_header("Access-Control-Allow-Origin", origin);
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "content-type");
    res.set_header("Access-Control-Allow-Credentials", "true"); // Enable cookies
    res.set_header("Access-Control-Max-Age", "86400");
    if (req.method == "OPTIONS") {
      res.status = 204;
      return httplib::Server::HandlerResponse::Handled;
    }
    return httplib::Server::HandlerResponse::Unhandled;
  });

  svr.Post("/", [&handler](const httplib::Request& req, httplib::Response& res) {
    handler.handleRequest(req,res);
  });

  // ---------- Start server ----------
  std::cout << "Serving server on http://127.0.0.1:5000\n";
  svr.listen("0.0.0.0", 5000);
}
