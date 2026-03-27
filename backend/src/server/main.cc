// #define WIN32_LEAN_AND_MEAN
// #define NOMINMAX
// #define _WIN32_WINNT 0x0A00
// these are not needed because they are in cmakelists

// #include "../virtualRenderer/virtualRenderer.h"  // Add this include
#include "../virtualRenderer/MiddleendRequestHandler.h"
#include "../../../src/gen/cpp/client_message.pb.h"

#include "httplib.h"
#include <google/protobuf/text_format.h>
#include <iostream>
#include "../utils/Log.h"

int main() {
  httplib::Server svr;
  MiddleendRequestHandler handler;

  // ---------- CORS handling for cross-origin + credentialed requests ----------
  svr.set_pre_routing_handler([](const httplib::Request& req, httplib::Response& res) {
    const std::string origin = req.get_header_value("Origin");
    if (!origin.empty()) {
      res.set_header("Access-Control-Allow-Origin", origin);
      res.set_header("Access-Control-Allow-Credentials", "true");
      res.set_header("Vary", "Origin");
    }

    res.set_header("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
    res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");

    if (req.method == "OPTIONS") {
      res.status = 204;
      return httplib::Server::HandlerResponse::Handled;
    }

    return httplib::Server::HandlerResponse::Unhandled;
  });

  svr.Post("/clientmessage", [&handler](const httplib::Request& req, httplib::Response& res) {
    handler.handleRequest(req, res);
  });

  // ---------- Start server ----------
  Log::info("Serving server on http://0.0.0.0:8080");
  svr.listen("0.0.0.0", 8080);
}
