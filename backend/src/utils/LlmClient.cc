#include "LlmClient.h"
#include "Log.h"
#include "../server/httplib.h"
#include <jsoncons/json.hpp>
#include <chrono>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace {

// Defaults are the local model the claim parser has always used, so an
// unconfigured checkout keeps talking to exactly the same endpoint.
constexpr const char* kDefaultHost = "192.168.86.124";
constexpr int kDefaultPort = 8002;
constexpr const char* kDefaultPath = "/v1/chat/completions";
constexpr const char* kDefaultModel = "Qwen3.6-35B-A3B-UD-Q3_K_XL.gguf";
constexpr int kDefaultMaxTokens = 4000;
constexpr int kDefaultReadTimeoutSeconds = 120;
constexpr int kConnectTimeoutSeconds = 10;

struct LlmConfig {
    std::string host = kDefaultHost;
    int port = kDefaultPort;
    std::string path = kDefaultPath;
    std::string model = kDefaultModel;
    int maxTokens = kDefaultMaxTokens;
    int readTimeoutSeconds = kDefaultReadTimeoutSeconds;
    std::string apiKey;
};

std::string envString(const char* name, const std::string& fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    return std::string(value);
}

// Falls back rather than throwing on a non-numeric value: a typo in an env var
// should not take the server down at first use, and the warning says what
// happened.
int envInt(const char* name, int fallback) {
    const char* value = std::getenv(name);
    if (value == nullptr || value[0] == '\0') {
        return fallback;
    }
    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        Log::warn(std::string("[LlmClient] ") + name + " is not a number ('" + value +
                  "'); using " + std::to_string(fallback));
        return fallback;
    }
}

// Re-read on every call rather than cached in a static. A handful of getenv
// lookups is nothing beside the network round trip that follows, and caching
// would mean the endpoint could never be changed or overridden after the first
// call -- including from a test, which is exactly where that matters.
LlmConfig config() {
    LlmConfig c;
    c.host = envString("DEBATE_LLM_HOST", kDefaultHost);
    c.port = envInt("DEBATE_LLM_PORT", kDefaultPort);
    c.path = envString("DEBATE_LLM_PATH", kDefaultPath);
    c.model = envString("DEBATE_LLM_MODEL", kDefaultModel);
    c.maxTokens = envInt("DEBATE_LLM_MAX_TOKENS", kDefaultMaxTokens);
    c.readTimeoutSeconds = envInt("DEBATE_LLM_TIMEOUT_SEC", kDefaultReadTimeoutSeconds);
    c.apiKey = envString("DEBATE_LLM_API_KEY", "");
    return c;
}

// A host carrying its own scheme ("https://api.example.com") is passed to
// httplib whole, which then handles TLS and the port itself; a bare host keeps
// the host+port form. This is what lets a hosted provider be dropped in later
// without touching any caller.
std::unique_ptr<httplib::Client> makeClient(const LlmConfig& cfg) {
    if (cfg.host.find("://") != std::string::npos) {
        return std::make_unique<httplib::Client>(cfg.host);
    }
    return std::make_unique<httplib::Client>(cfg.host, cfg.port);
}

// Strips a ```json ... ``` (or bare ``` ... ```) fence if the model wrapped its
// output in one despite being told not to.
std::string stripCodeFence(const std::string& text) {
    size_t start = text.find("```");
    if (start == std::string::npos) {
        return text;
    }
    size_t contentStart = text.find('\n', start);
    if (contentStart == std::string::npos) {
        return text;
    }
    contentStart += 1;
    size_t end = text.find("```", contentStart);
    if (end == std::string::npos) {
        return text;
    }
    return text.substr(contentStart, end - contentStart);
}

} // namespace

std::string LlmClient::DescribeEndpoint() {
    const LlmConfig cfg = config();
    std::string where = cfg.host;
    if (cfg.host.find("://") == std::string::npos) {
        where += ":" + std::to_string(cfg.port);
    }
    return where + cfg.path + " (" + cfg.model + ")";
}

std::string LlmClient::Chat(const std::vector<LlmMessage>& messages, const LlmOptions& options) {
    if (messages.empty()) {
        throw std::runtime_error("LlmClient: refusing to send a chat with no messages");
    }

    const LlmConfig cfg = config();
    const std::string model = options.model.empty() ? cfg.model : options.model;
    const int maxTokens = options.maxTokens > 0 ? options.maxTokens : cfg.maxTokens;

    jsoncons::json body;
    body["model"] = model;
    body["max_tokens"] = maxTokens;
    if (options.temperature >= 0) {
        body["temperature"] = options.temperature;
    }
    body["messages"] = jsoncons::json::array();
    for (const LlmMessage& message : messages) {
        jsoncons::json entry;
        entry["role"] = message.role;
        entry["content"] = message.content;
        body["messages"].push_back(entry);
    }

    std::ostringstream bodyStream;
    bodyStream << body;
    const std::string payload = bodyStream.str();

    std::unique_ptr<httplib::Client> cli = makeClient(cfg);
    cli->set_connection_timeout(kConnectTimeoutSeconds, 0);
    cli->set_read_timeout(cfg.readTimeoutSeconds, 0);

    httplib::Headers headers;
    if (!cfg.apiKey.empty()) {
        headers.emplace("Authorization", "Bearer " + cfg.apiKey);
    }

    Log::info("[LlmClient] " + options.label + ": POST " + DescribeEndpoint() + " (" +
              std::to_string(payload.size()) + " byte request)");
    const auto started = std::chrono::steady_clock::now();

    auto res = cli->Post(cfg.path, headers, payload, "application/json");

    const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::steady_clock::now() - started).count();

    if (!res) {
        throw std::runtime_error("LlmClient: " + options.label + " could not reach the model at " +
                                 DescribeEndpoint() + " (connection error after " +
                                 std::to_string(elapsedMs) + "ms)");
    }
    if (res->status != 200) {
        throw std::runtime_error("LlmClient: " + options.label + " got HTTP " +
                                 std::to_string(res->status) + " from " + DescribeEndpoint() +
                                 ": " + res->body);
    }

    std::string content = ExtractContent(res->body);
    Log::info("[LlmClient] " + options.label + ": " + std::to_string(content.size()) +
              " chars in " + std::to_string(elapsedMs) + "ms");
    return content;
}

std::string LlmClient::Complete(const std::string& prompt, const LlmOptions& options) {
    return Chat({LlmMessage{"user", prompt}}, options);
}

std::string LlmClient::ChatJson(const std::vector<LlmMessage>& messages, const LlmOptions& options) {
    const std::string content = Chat(messages, options);
    const std::string candidate = stripCodeFence(content);

    jsoncons::json parsed;
    try {
        parsed = jsoncons::json::parse(candidate);
    } catch (const std::exception&) {
        // The model's own words are the useful diagnostic here -- it usually
        // explains itself in prose when it declines to answer in JSON.
        throw std::runtime_error("LlmClient: " + options.label +
                                 " did not return valid JSON. First 200 chars: " +
                                 content.substr(0, 200));
    }

    std::ostringstream out;
    out << jsoncons::pretty_print(parsed);
    return out.str();
}

std::string LlmClient::CompleteJson(const std::string& prompt, const LlmOptions& options) {
    return ChatJson({LlmMessage{"user", prompt}}, options);
}

std::string LlmClient::ExtractContent(const std::string& responseBody) {
    jsoncons::json outer;
    try {
        outer = jsoncons::json::parse(responseBody);
    } catch (const std::exception& e) {
        throw std::runtime_error("LlmClient: response envelope wasn't valid JSON: " +
                                 std::string(e.what()));
    }

    if (!outer.contains("choices") || outer["choices"].empty()) {
        throw std::runtime_error("LlmClient: response had no 'choices'");
    }
    return outer["choices"][0]["message"]["content"].as<std::string>();
}
