#define _WIN32_WINNT 0x0A00
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#ifndef WINAPI_FAMILY
#define WINAPI_FAMILY WINAPI_FAMILY_DESKTOP_APP
#endif

// #define CPPHTTPLIB_NO_FILESYSTEM
#include "httplib.h"
#include <iostream>

int main() {
  std::cout << "Starting minimal server...\n";
  httplib::Server svr;
  svr.Get("/", [](const auto&, auto& res){ res.set_content("ok", "text/plain"); });

  std::cout << "Listening on http://127.0.0.1:8080 ...\n";
  bool ok = svr.listen("0.0.0.0", 8080);
  std::cout << "listen() returned " << ok << " errno=" << errno << "\n";
  return ok ? 0 : 1;
}
