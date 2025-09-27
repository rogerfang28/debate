// main.cpp
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00

#include "../../../src/gen/cpp/page.pb.h"
#include "../../../src/gen/cpp/event.pb.h"

#include "httplib.h"
#include <google/protobuf/text_format.h>
#include <windows.h>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>

static std::filesystem::path exe_dir() {
  wchar_t buf[MAX_PATH];
  DWORD n = GetModuleFileNameW(nullptr, buf, MAX_PATH);
  return std::filesystem::path(std::wstring(buf, n)).parent_path();
}

int main() {
  httplib::Server svr;

  svr.Get("/", [](const httplib::Request&, httplib::Response& res) {
    // Make path robust no matter how you launch the exe
    auto pbtxtPath = exe_dir() / ".." / "virtualRenderer" / "pages" / "schemas" / "testPage.pbtxt";
    pbtxtPath = std::filesystem::weakly_canonical(pbtxtPath);

    std::cerr << "Looking for pbtxt at: " << pbtxtPath.u8string() << "\n";
    std::ifstream input(pbtxtPath, std::ios::binary);
    if (!input) {
      res.status = 500;
      res.set_content("Failed to open homePage.pbtxt", "text/plain");
      std::cerr << "ERR: couldn't open " << pbtxtPath.u8string() << "\n";
      return;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    ui::Page page;
    if (!google::protobuf::TextFormat::ParseFromString(buffer.str(), &page)) {
      res.status = 500;
      res.set_content("Failed to parse pbtxt", "text/plain");
      std::cerr << "ERR: TextFormat parse failed\n";
      return;
    }

    std::string out;
    page.SerializeToString(&out);
    res.set_content(out, "application/x-protobuf"); // or "application/octet-stream"
    std::cout << "GET / served homePage.pbtxt (" << out.size() << " bytes)\n";
  });

  svr.Post("/", [](const httplib::Request&, httplib::Response& res){
    res.set_content("got post?", "text/plain");
    std::cout << "POST /\n";
  });

  std::cout << "Serving on http://127.0.0.1:8080\n";
  svr.listen("0.0.0.0", 8080);
}
