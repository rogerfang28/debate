#pragma once

#include <string>
#include <vector>

// The one place the project talks to a language model.
//
// Every AI-backed feature -- extracting claims from an article, summarizing a
// debate, drafting a debate -- goes through here. Each of those differs only in
// its PROMPT and in what it does with the answer; the transport, the request
// shape, the config, the error handling and the "did it actually return JSON"
// check are the same every time. Keeping one client means a feature is a prompt
// plus a parser, and means switching model or host is one change rather than
// one per caller.
//
// The endpoint is any OpenAI-compatible /v1/chat/completions server -- llama.cpp,
// Ollama, vLLM, or a hosted provider -- configured by environment variable:
//
//   DEBATE_LLM_HOST         default 192.168.86.124  (or a full https://... URL)
//   DEBATE_LLM_PORT         default 8002            (ignored if HOST has a scheme)
//   DEBATE_LLM_PATH         default /v1/chat/completions
//   DEBATE_LLM_MODEL        default Qwen3.6-35B-A3B-UD-Q3_K_XL.gguf
//   DEBATE_LLM_MAX_TOKENS   default 4000
//   DEBATE_LLM_TIMEOUT_SEC  default 120 (read timeout; connect is fixed at 10s)
//   DEBATE_LLM_API_KEY      optional; sent as "Authorization: Bearer ..."
//
// Defaults reproduce the original hardcoded local endpoint, so an unconfigured
// checkout behaves exactly as before.
//
// Every call is SYNCHRONOUS and can block for the read timeout. Do not call it
// on a path that renders a page -- these belong on explicit user-triggered
// actions, ideally off the request thread.

// One turn of a conversation. `role` is "system", "user", or "assistant".
struct LlmMessage {
    std::string role;
    std::string content;
};

// Per-call overrides. Anything left at its default falls back to the
// environment config, so most callers can pass nothing at all.
struct LlmOptions {
    std::string model;        // empty -> configured default
    int maxTokens = 0;        // <= 0  -> configured default
    double temperature = -1;  // <  0  -> omit, let the server decide
    // Shown in logs so a slow or failing call can be traced to the feature
    // that made it rather than just to "the LLM".
    std::string label = "llm";
};

class LlmClient {
public:
    // Send a single user prompt, get the assistant's reply text.
    static std::string Complete(const std::string& prompt, const LlmOptions& options = {});

    // Full control: system messages, few-shot examples, multi-turn.
    static std::string Chat(const std::vector<LlmMessage>& messages, const LlmOptions& options = {});

    // For prompts that demand JSON back. Strips a ``` fence if the model added
    // one anyway, parses the result to prove it is valid, and returns it
    // pretty-printed. Throws if the reply is not parseable JSON -- so callers
    // can assume what they receive is well-formed and only have to check that
    // the SHAPE is what they asked for.
    static std::string CompleteJson(const std::string& prompt, const LlmOptions& options = {});
    static std::string ChatJson(const std::vector<LlmMessage>& messages, const LlmOptions& options = {});

    // "host:port/path (model)" -- for logs and error messages.
    static std::string DescribeEndpoint();

private:
    // Pulls the assistant text out of an OpenAI-style chat-completion envelope.
    static std::string ExtractContent(const std::string& responseBody);
};
