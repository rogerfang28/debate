#include "UrlFetcher.h"

#include "Log.h"
#include "../server/httplib.h"

#include <algorithm>
#include <cctype>
#include <cstring>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#endif

namespace {

constexpr size_t kMaxBytes = 2 * 1024 * 1024;   // 2 MB of HTML is a long article
constexpr int kConnectTimeoutSeconds = 5;
constexpr int kReadTimeoutSeconds = 15;
constexpr int kMaxRedirects = 3;

struct ParsedUrl {
    bool ok = false;
    bool https = false;
    std::string host;
    int port = 0;
    std::string path;
};

ParsedUrl parseUrl(const std::string& url) {
    ParsedUrl out;
    std::string rest;
    if (url.rfind("https://", 0) == 0) {
        out.https = true;
        rest = url.substr(8);
    } else if (url.rfind("http://", 0) == 0) {
        out.https = false;
        rest = url.substr(7);
    } else {
        return out;  // Only http(s). file://, gopher://, etc. are refused.
    }

    const size_t slash = rest.find('/');
    std::string hostPort = slash == std::string::npos ? rest : rest.substr(0, slash);
    out.path = slash == std::string::npos ? "/" : rest.substr(slash);

    // Credentials in the authority ("user@host") are dropped: they are a
    // classic way to make a URL read as one host while resolving to another.
    const size_t at = hostPort.find('@');
    if (at != std::string::npos) {
        hostPort = hostPort.substr(at + 1);
    }

    const size_t colon = hostPort.rfind(':');
    if (colon != std::string::npos && hostPort.find(']') == std::string::npos) {
        out.host = hostPort.substr(0, colon);
        out.port = std::atoi(hostPort.substr(colon + 1).c_str());
    } else {
        out.host = hostPort;
        out.port = out.https ? 443 : 80;
    }

    if (out.host.empty() || out.port <= 0) {
        return out;
    }
    out.ok = true;
    return out;
}


void replaceEntities(std::string& text) {
    const std::pair<const char*, const char*> entities[] = {
        {"&nbsp;", " "},   {"&amp;", "&"},    {"&lt;", "<"},      {"&gt;", ">"},
        {"&quot;", "\""}, {"&#39;", "'"},    {"&rsquo;", "'"},   {"&lsquo;", "'"},
        {"&ldquo;", "\""}, {"&rdquo;", "\""}, {"&mdash;", " - "}, {"&ndash;", "-"},
    };
    for (const auto& entity : entities) {
        size_t at = 0;
        const size_t toLen = std::strlen(entity.second);
        while ((at = text.find(entity.first, at)) != std::string::npos) {
            text.replace(at, std::strlen(entity.first), entity.second);
            at += toLen;
        }
    }
}

// Collapses runs of whitespace but keeps paragraph breaks, so the model sees
// prose rather than one continuous line.
std::string collapseWhitespace(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    bool lastWasSpace = false;
    int newlineRun = 0;
    for (char c : text) {
        if (c == '\n') {
            newlineRun++;
            lastWasSpace = true;
            continue;
        }
        if (std::isspace(static_cast<unsigned char>(c))) {
            lastWasSpace = true;
            continue;
        }
        if (newlineRun > 0) {
            out.append(newlineRun > 1 ? "\n\n" : "\n");
            newlineRun = 0;
        } else if (lastWasSpace && !out.empty()) {
            out.push_back(' ');
        }
        lastWasSpace = false;
        out.push_back(c);
    }
    return out;
}

} // namespace

bool UrlFetcher::IsBlockedAddress(const std::string& ipLiteral) {
    in_addr v4{};
    if (inet_pton(AF_INET, ipLiteral.c_str(), &v4) == 1) {
        const uint32_t host = ntohl(v4.s_addr);
        const uint8_t a = static_cast<uint8_t>((host >> 24) & 0xFF);
        const uint8_t b = static_cast<uint8_t>((host >> 16) & 0xFF);

        if (a == 0) return true;                          // this network
        if (a == 10) return true;                         // private
        if (a == 127) return true;                        // loopback
        if (a == 169 && b == 254) return true;            // link-local, incl. cloud metadata
        if (a == 172 && b >= 16 && b <= 31) return true;  // private
        if (a == 192 && b == 168) return true;            // private -- the LAN this box is on
        if (a == 100 && b >= 64 && b <= 127) return true; // carrier-grade NAT
        if (a >= 224) return true;                        // multicast and reserved
        return false;
    }

    in6_addr v6{};
    if (inet_pton(AF_INET6, ipLiteral.c_str(), &v6) == 1) {
        const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&v6);
        const bool allZeroButLast =
            std::all_of(bytes, bytes + 15, [](uint8_t byte) { return byte == 0; });
        if (allZeroButLast && bytes[15] == 1) return true;   // ::1 loopback
        if (allZeroButLast && bytes[15] == 0) return true;   // ::
        if (bytes[0] == 0xFE && (bytes[1] & 0xC0) == 0x80) return true;  // fe80:: link-local
        if ((bytes[0] & 0xFE) == 0xFC) return true;          // fc00::/7 unique local
        if (bytes[0] == 0xFF) return true;                   // multicast
        // IPv4-mapped (::ffff:a.b.c.d) must be judged by its IPv4 part.
        const bool mapped = std::all_of(bytes, bytes + 10, [](uint8_t byte) { return byte == 0; }) &&
                            bytes[10] == 0xFF && bytes[11] == 0xFF;
        if (mapped) {
            char buf[INET_ADDRSTRLEN] = {0};
            in_addr inner{};
            std::memcpy(&inner, bytes + 12, 4);
            if (inet_ntop(AF_INET, &inner, buf, sizeof(buf))) {
                return IsBlockedAddress(buf);
            }
            return true;
        }
        return false;
    }

    // Not an address we can classify. Refusing is the safe direction.
    return true;
}

std::string UrlFetcher::HtmlToText(const std::string& html) {
    // Script and style hold code, not prose; their contents would otherwise
    // reach the model prompt as a wall of noise.
    std::string stripped;
    stripped.reserve(html.size());
    size_t i = 0;
    while (i < html.size()) {
        const size_t open = html.find('<', i);
        if (open == std::string::npos) {
            stripped.append(html, i, std::string::npos);
            break;
        }
        stripped.append(html, i, open - i);

        std::string lower = html.substr(open, 8);
        std::transform(lower.begin(), lower.end(), lower.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        const char* skipTo = nullptr;
        if (lower.rfind("<script", 0) == 0) {
            skipTo = "</script";
        } else if (lower.rfind("<style", 0) == 0) {
            skipTo = "</style";
        }
        if (skipTo != nullptr) {
            const size_t end = html.find(skipTo, open);
            i = (end == std::string::npos) ? html.size() : end;
            continue;
        }

        const size_t close = html.find('>', open);
        if (close == std::string::npos) {
            break;
        }

        // A '<' is not necessarily a tag. Prose contains comparisons -- "under
        // 5 < 10 pounds" -- and treating those as markup swallows everything up
        // to the next '>', deleting real sentences invisibly. A real tag starts
        // with a letter or a slash; anything else is text.
        const char afterAngle = (open + 1 < html.size()) ? html[open + 1] : ' ';
        const bool looksLikeTag =
            std::isalpha(static_cast<unsigned char>(afterAngle)) || afterAngle == '/' ||
            afterAngle == '!' || afterAngle == '?';
        if (!looksLikeTag) {
            stripped.push_back('<');
            i = open + 1;
            continue;
        }
        // Block tags become breaks so sentences do not run together.
        if (lower.rfind("<p", 0) == 0 || lower.rfind("<br", 0) == 0 ||
            lower.rfind("<div", 0) == 0 || lower.rfind("<h1", 0) == 0 ||
            lower.rfind("<h2", 0) == 0 || lower.rfind("<h3", 0) == 0 ||
            lower.rfind("<li", 0) == 0 || lower.rfind("</p", 0) == 0) {
            stripped.push_back('\n');
        }
        i = close + 1;
    }

    replaceEntities(stripped);
    return collapseWhitespace(stripped);
}

UrlFetcher::Result UrlFetcher::Fetch(const std::string& url) {
    Result result;
    try {
        return FetchInner(url);
    } catch (const std::exception& e) {
        // httplib throws from its own constructor for an https URL when SSL is
        // not compiled in, and can throw on malformed input. A fetch that fails
        // must come back as an error the user can read, never as an exception
        // unwinding through a request.
        Log::warn(std::string("[UrlFetcher] Fetch threw: ") + e.what());
        result.error = std::string("Could not fetch that link: ") + e.what();
        return result;
    } catch (...) {
        result.error = "Could not fetch that link.";
        return result;
    }
}

UrlFetcher::Result UrlFetcher::FetchInner(const std::string& url) {
    Result result;
    std::string current = url;

    for (int hop = 0; hop <= kMaxRedirects; ++hop) {
        const ParsedUrl parsed = parseUrl(current);
        if (!parsed.ok) {
            result.error = "That does not look like an http or https link.";
            return result;
        }

        // RESOLVE FIRST, THEN JUDGE THE ADDRESS. The hostname proves nothing:
        // a name under someone else's control can point at 127.0.0.1 or at
        // anything on this machine's own network.
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_STREAM;
        addrinfo* info = nullptr;
        if (getaddrinfo(parsed.host.c_str(), nullptr, &hints, &info) != 0 || info == nullptr) {
            result.error = "Could not look up that address.";
            return result;
        }

        bool blocked = false;
        for (addrinfo* it = info; it != nullptr && !blocked; it = it->ai_next) {
            char buf[INET6_ADDRSTRLEN] = {0};
            if (it->ai_family == AF_INET) {
                inet_ntop(AF_INET, &reinterpret_cast<sockaddr_in*>(it->ai_addr)->sin_addr, buf,
                          sizeof(buf));
            } else if (it->ai_family == AF_INET6) {
                inet_ntop(AF_INET6, &reinterpret_cast<sockaddr_in6*>(it->ai_addr)->sin6_addr, buf,
                          sizeof(buf));
            } else {
                blocked = true;
                break;
            }
            // EVERY address a name resolves to must be public. One private
            // answer is enough to refuse -- which address actually gets
            // connected to is not something this code decides.
            if (IsBlockedAddress(buf)) {
                blocked = true;
            }
        }
        freeaddrinfo(info);

        if (blocked) {
            Log::warn("[UrlFetcher] Refused a link resolving to a private address: " + parsed.host);
            result.error =
                "That link points inside a private network, so it will not be fetched. Paste the "
                "text instead.";
            return result;
        }

        const std::string base = (parsed.https ? "https://" : "http://") + parsed.host + ":" +
                                 std::to_string(parsed.port);
        httplib::Client client(base);
        client.set_connection_timeout(kConnectTimeoutSeconds, 0);
        client.set_read_timeout(kReadTimeoutSeconds, 0);
        // Redirects are followed by hand so every destination is re-checked.
        client.set_follow_location(false);

        auto response = client.Get(parsed.path.c_str());
        if (!response) {
            result.error = "Could not reach that page.";
            return result;
        }

        if (response->status >= 300 && response->status < 400) {
            const std::string next = response->get_header_value("Location");
            if (next.empty()) {
                result.error = "That page redirected without saying where.";
                return result;
            }
            current = (next.rfind("http", 0) == 0) ? next : base + next;
            continue;
        }
        if (response->status != 200) {
            result.error = "That page returned HTTP " + std::to_string(response->status) + ".";
            return result;
        }
        if (response->body.size() > kMaxBytes) {
            result.error = "That page is too large to read.";
            return result;
        }

        result.text = HtmlToText(response->body);
        if (result.text.size() < 200) {
            result.error =
                "There was not enough readable text on that page. It may need JavaScript, or sit "
                "behind a paywall. Paste the text instead.";
            return result;
        }
        result.finalUrl = current;
        result.ok = true;
        Log::info("[UrlFetcher] Fetched " + std::to_string(result.text.size()) + " chars from " +
                  parsed.host);
        return result;
    }

    result.error = "That link redirected too many times.";
    return result;
}
