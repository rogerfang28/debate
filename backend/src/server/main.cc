// #define WIN32_LEAN_AND_MEAN
// #define NOMINMAX
// #define _WIN32_WINNT 0x0A00
// these are not needed because they are in cmakelists

// #include "../virtualRenderer/virtualRenderer.h"  // Add this include
#include "../virtualRenderer/MiddleendRequestHandler.h"
#include "client_message.pb.h"

#include "httplib.h"
#include <google/protobuf/text_format.h>
#include <cstdlib>
#include <iostream>
#include "../utils/Log.h"
#include "../utils/GoogleJWTVerifier.h"
#include "../utils/SessionToken.h"
#include <string>

int main() {
  // Sessions are signed with DEBATE_SESSION_SECRET. A production server
  // (DEBATE_ENV=production) will not start without one: with a random
  // per-run secret every restart would sign everyone out, and a missing
  // setting should be loud, not quietly different. Elsewhere a random
  // secret is fine and ServerSecret() says so in the log.
  if (const char* env = std::getenv("DEBATE_ENV"); env != nullptr && std::string(env) == "production" &&
      !session_token::SecretIsConfigured()) {
    Log::error("[Session] DEBATE_ENV=production but DEBATE_SESSION_SECRET is missing or shorter than " +
               std::to_string(session_token::kMinSecretLength) + " characters. Refusing to start.");
    return 1;
  }
  session_token::ServerSecret();  // settle the secret, and log about it, before the first request

  httplib::Server svr;
  MiddleendRequestHandler handler;
  // Google accounts named by their email get a username of their own, and
  // old visitors are swept -- now, and every few hours after.
  handler.startBackgroundWork();

  // Pre-warm Google's JWKS cache at startup so the first login doesn't pay
  // the fetch latency, and the timestamp check in verify() has a real
  // baseline instead of defaulting to epoch 0. Non-fatal if it fails (e.g.
  // no network, or Google Sign-In isn't configured) — verify() will retry
  // the fetch on first use.
  if (std::getenv("GOOGLE_CLIENT_ID") != nullptr) {
    if (!GoogleJWTVerifier::init()) {
      Log::error("[GoogleAuth] Failed to pre-load JWKS at startup; will retry on first login.");
    }
  }

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
  const char* HOST = std::getenv("DEBATE_SERVER_HOST");
  if (HOST == nullptr || HOST[0] == '\0') {
    HOST = "127.0.0.1";
  }

  const char* portEnv = std::getenv("DEBATE_SERVER_PORT");
  const int PORT = (portEnv != nullptr && portEnv[0] != '\0') ? std::atoi(portEnv) : 8080;

  Log::info(std::string("Attempting to bind server to http://") + HOST + ":" + std::to_string(PORT));

  if (svr.bind_to_port(HOST, PORT) < 0) {
    Log::error(std::string("Failed to bind ") + HOST + ":" + std::to_string(PORT) +
               " - port may be in use or blocked");
    return 1;
  }

  Log::info(std::string("Server is listening on http://") + HOST + ":" + std::to_string(PORT));
  if (!svr.listen_after_bind()) {
    Log::error("Server stopped unexpectedly.");
    return 1;
  }

  return 0;
}
