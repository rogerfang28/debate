// #define WIN32_LEAN_AND_MEAN
// #define NOMINMAX
// #define _WIN32_WINNT 0x0A00
// these are not needed because they are in cmakelists

#include "../debate/DebateModerator.h"
#include "../../../src/gen/cpp/debate_event.pb.h"
#include "BackendRequestHandler.h"

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
  BackendRequestHandler handler;

  // ---------- CORS middleware ----------
  svr.set_pre_routing_handler([](const httplib::Request &req, httplib::Response &res) {
    // Allow credentials (cookies) - must specify exact origin, not "*"
    std::string origin = req.get_header_value("Origin");
    if (origin.empty()) {
      origin = "http://localhost:5173"; // Default for Vite dev server
    }
    
    // Allow requests from middleend (5000)
    if (origin == "http://localhost:5000"){ 
      res.set_header("Access-Control-Allow-Origin", origin);
    } else {
      res.set_header("Access-Control-Allow-Origin", origin);
    }
    
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

  // ---------- GET / ----------
  svr.Get("/", [&handler](const httplib::Request& req, httplib::Response& res) {
    // renderer.handleGetRequest(req, res);
    handler.handleGetRequest(req, res);
  });

  svr.Post("/", [&handler](const httplib::Request& req, httplib::Response& res) {
    // renderer.handleClientMessage(req,res);
    handler.handlePostRequest(req, res);
  });

  // ---------- Start server ----------
  std::cout << "Serving server on http://127.0.0.1:8080\n";
  svr.listen("0.0.0.0", 8080);
}
