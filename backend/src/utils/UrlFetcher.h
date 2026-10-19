#pragma once

#include <string>

// Fetches a web page and returns its readable text.
//
// THIS IS THE ONE PLACE THE SERVER FETCHES A URL A USER CHOSE, which makes it
// the app's server-side request forgery surface. The danger of a "malicious
// link" here is not what the page contains -- it is where the link POINTS. A
// server that fetches anything asked of it becomes a proxy into its own
// network, and this server can reach the local model endpoint, the loopback
// interface, and every device on the LAN. Anyone using the public demo would
// be borrowing that reach.
//
// So the address is checked, not the URL string: the hostname is resolved and
// the resulting IP rejected if it is loopback, private, link-local, multicast
// or otherwise not a public address. Checking the text of the URL is not
// enough -- "http://myhost.example" can resolve to 127.0.0.1, and a redirect
// can land somewhere the original URL never mentioned, so each hop is
// re-checked.
//
// A residual risk remains that no address check can remove: the page text goes
// into a model prompt, and a page can contain text written to look like
// instructions. Fetched text is data, never instruction, and the generation
// prompt is built so the page fills a slot rather than steering the request --
// but a determined page can still influence what claims come out. Treat a
// generated debate as coming from the source it was generated from.
class UrlFetcher {
public:
    struct Result {
        bool ok = false;
        std::string text;   // readable page text, tags stripped
        std::string error;  // shown to the user verbatim
        std::string finalUrl;
    };

    // Only http and https. Follows a small number of redirects, re-checking
    // the destination each time. Caps both the response size and the time
    // spent, because a hostile or merely broken server should not be able to
    // hold a request thread or fill memory.
    static Result Fetch(const std::string& url);

    // Exposed for tests: true if this address must not be fetched.
    static bool IsBlockedAddress(const std::string& ipLiteral);

    // Exposed for tests: crude HTML to text.
    static std::string HtmlToText(const std::string& html);

private:
    // Fetch() is a guard around this: it turns any exception into an error on
    // the Result, so callers never have to handle one.
    static Result FetchInner(const std::string& url);
};
