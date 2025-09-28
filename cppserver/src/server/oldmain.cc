// main.cpp
#define _WIN32_WINNT 0x0A00

#include "../../../src/gen/cpp/page.pb.h"   // ui::Page (your existing page proto)
#include "../../../src/gen/cpp/event.pb.h"  // debate::UIEvent (your schema shown)
#include <google/protobuf/text_format.h>
#include "../main/eventHandler.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#include <cstdio>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

// ---------- tiny utils ----------
static bool send_all(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(s, buf + sent, len - sent, 0);
        if (n == SOCKET_ERROR) return false;
        sent += n;
    }
    return true;
}

static bool load_file_to_string(const std::string& path, std::string& out) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    std::ostringstream ss;
    ss << f.rdbuf();
    out = ss.str();
    return true;
}

static std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c){ return (char)std::tolower(c); });
    return s;
}

static std::string get_header(const std::string& headers, const std::string& key) {
    std::string k = to_lower(key);
    size_t start = 0;
    while (start < headers.size()) {
        size_t end = headers.find("\r\n", start);
        std::string line = headers.substr(start, (end == std::string::npos ? headers.size() : end) - start);
        if (line.empty()) break;
        auto colon = line.find(':');
        if (colon != std::string::npos) {
            std::string name = to_lower(line.substr(0, colon));
            if (name == k) {
                size_t vpos = colon + 1;
                while (vpos < line.size() && (line[vpos] == ' ' || line[vpos] == '\t')) vpos++;
                return line.substr(vpos);
            }
        }
        if (end == std::string::npos) break;
        start = end + 2;
    }
    return "";
}

static bool starts_with(const std::string& s, const char* pfx) {
    return s.rfind(pfx, 0) == 0;
}

// ---------- program ----------
int main() {
    // 1) Load and parse pbtxt at startup for the Page
    std::string pbtxt;
    const std::string pbtxt_path = "../virtualRenderer/pages/schemas/testPage.pbtxt"; // adjust if needed

    if (!load_file_to_string(pbtxt_path, pbtxt)) {
        std::fprintf(stderr, "Failed to read %s\n", pbtxt_path.c_str());
        return 100;
    }

    ui::Page page;
    if (!google::protobuf::TextFormat::ParseFromString(pbtxt, &page)) {
        std::fprintf(stderr, "Failed to parse text-format protobuf from %s\n", pbtxt_path.c_str());
        return 101;
    }

    std::string page_bin;
    if (!page.SerializeToString(&page_bin)) {
        std::fprintf(stderr, "Failed to serialize Page to binary\n");
        return 102;
    }

    // 2) Winsock init
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) return 1;

    SOCKET srv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (srv == INVALID_SOCKET) { WSACleanup(); return 2; }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::perror("bind");
        closesocket(srv); WSACleanup(); return 3;
    }
    if (listen(srv, SOMAXCONN) == SOCKET_ERROR) {
        std::perror("listen");
        closesocket(srv); WSACleanup(); return 4;
    }

    std::printf("Listening on http://127.0.0.1:8080 ...\n");

    // 3) Accept loop
    for (;;) {
        SOCKET cli = accept(srv, nullptr, nullptr);
        if (cli == INVALID_SOCKET) break;

        // Read until headers complete
        std::string req;
        char buf[4096];
        int n;
        size_t header_end_pos = std::string::npos;
        while ((n = recv(cli, buf, sizeof(buf), 0)) > 0) {
            req.append(buf, buf + n);
            header_end_pos = req.find("\r\n\r\n");
            if (header_end_pos != std::string::npos) break;
        }
        if (header_end_pos == std::string::npos) { shutdown(cli, SD_BOTH); closesocket(cli); continue; }

        const std::string headers = req.substr(0, header_end_pos + 2);
        const std::string request_line = headers.substr(0, headers.find("\r\n"));
        bool is_get  = starts_with(request_line, "GET ");
        bool is_post = starts_with(request_line, "POST ");
        bool is_opt  = starts_with(request_line, "OPTIONS ");

        // Extract path (METHOD <path> HTTP/1.1)
        std::string path = "/";
        {
            size_t sp1 = request_line.find(' ');
            size_t sp2 = (sp1 == std::string::npos) ? std::string::npos : request_line.find(' ', sp1 + 1);
            if (sp1 != std::string::npos && sp2 != std::string::npos) {
                path = request_line.substr(sp1 + 1, sp2 - sp1 - 1);
            }
        }

        // Helper to send CORS responses
        auto send_cors = [&](const char* status_line, const char* extra_headers, const char* body, size_t body_len) {
            std::ostringstream oss;
            oss << status_line
                << "Access-Control-Allow-Origin: *\r\n"
                << "Access-Control-Allow-Methods: GET, POST, OPTIONS\r\n"
                << "Access-Control-Allow-Headers: content-type\r\n"
                << "Access-Control-Max-Age: 86400\r\n";
            if (extra_headers) oss << extra_headers;
            oss << "Content-Length: " << body_len << "\r\n"
                << "Connection: close\r\n\r\n";
            const std::string hdr = oss.str();
            send_all(cli, hdr.data(), (int)hdr.size());
            if (body_len) send_all(cli, body, (int)body_len);
        };

        // OPTIONS preflight
        if (is_opt) {
            send_cors("HTTP/1.1 204 No Content\r\n", nullptr, "", 0);
            shutdown(cli, SD_BOTH); closesocket(cli); continue;
        }

        // GET /
        if (is_get && path == "/") {
            std::ostringstream extra;
            std::cout << "received get" << "\n";
            extra << "Content-Type: application/x-protobuf\r\n";
            send_cors("HTTP/1.1 200 OK\r\n", extra.str().c_str(), page_bin.data(), page_bin.size());
            shutdown(cli, SD_BOTH); closesocket(cli); continue;
        }

        // POST /  (expects debate.UIEvent)
        if (is_post && path == "/") {
            // Content-Length
            std::cout << "received post" << "\n";
            std::string cl = get_header(headers, "Content-Length");
            size_t content_len = cl.empty() ? 0 : (size_t)std::stoul(cl);

            // Content-Type
            std::string ct = to_lower(get_header(headers, "Content-Type"));

            // Collect body (some bytes may already be in 'req')
            size_t body_start = header_end_pos + 4;
            std::string body = (body_start < req.size()) ? req.substr(body_start) : std::string();
            while (body.size() < content_len) {
                n = recv(cli, buf, sizeof(buf), 0);
                if (n <= 0) break;
                body.append(buf, buf + n);
            }

            if (body.size() != content_len) {
                const char* msg = "Bad Request: body truncated\n";
                send_cors("HTTP/1.1 400 Bad Request\r\n", "Content-Type: text/plain; charset=utf-8\r\n", msg, std::strlen(msg));
                shutdown(cli, SD_BOTH); closesocket(cli); continue;
            }

            if (ct.find("application/x-protobuf") == std::string::npos) {
                const char* msg = "Unsupported Media Type: expected application/x-protobuf\n";
                send_cors("HTTP/1.1 415 Unsupported Media Type\r\n", "Content-Type: text/plain; charset=utf-8\r\n", msg, std::strlen(msg));
                shutdown(cli, SD_BOTH); closesocket(cli); continue;
            }

            // Parse as debate.UIEvent
            debate::UIEvent evt;
            if (!evt.ParseFromArray(body.data(), (int)body.size())) {
                const char* msg = "Bad Request: failed to parse debate.UIEvent\n";
                send_cors("HTTP/1.1 400 Bad Request\r\n", "Content-Type: text/plain; charset=utf-8\r\n", msg, std::strlen(msg));
                shutdown(cli, SD_BOTH); closesocket(cli); continue;
            }

            // --- Handle the event using EventHandler ---
            EventHandler handler(std::move(evt));  // Move construct since EventHandler takes by value
            handler.Log(std::cout);  // Log the event details to console

            // Respond with 204 No Content (or change to 200 + body if you prefer)
            send_cors("HTTP/1.1 204 No Content\r\n", nullptr, "", 0);
            shutdown(cli, SD_BOTH); closesocket(cli); continue;
        }

        // Fallback 404
        {
            const char* notfound = "Not Found\n";
            send_cors("HTTP/1.1 404 Not Found\r\n", "Content-Type: text/plain; charset=utf-8\r\n", notfound, std::strlen(notfound));
        }

        shutdown(cli, SD_BOTH);
        closesocket(cli);
    }

    closesocket(srv);
    WSACleanup();
    return 0;
}
