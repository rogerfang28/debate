#include <gtest/gtest.h>

#include "../../backend/src/server/httplib.h"
#include "../../backend/src/utils/LlmClient.h"

#include <jsoncons/json.hpp>
#include <cstdlib>
#include <string>
#include <thread>

// ---------------------------------------
// LlmClient
// ---------------------------------------
// LlmClient is the single door to the model for every AI feature, so a mistake
// in it is a mistake in all of them at once. These tests stand a real
// OpenAI-compatible server up in-process and point the client at it, rather
// than mocking the transport: the things most likely to be wrong here are the
// exact shape of the request body and the exact path through the response
// envelope, and a mock would assert my assumptions instead of testing them.
//
// Nothing here touches the configured local model -- every test overrides the
// endpoint to 127.0.0.1, so the suite passes with that machine switched off.

namespace {

void setEnv(const char* name, const std::string& value) {
#ifdef _WIN32
    _putenv_s(name, value.c_str());
#else
    setenv(name, value.c_str(), 1);
#endif
}

void clearEnv(const char* name) {
#ifdef _WIN32
    _putenv_s(name, "");
#else
    unsetenv(name);
#endif
}

// A stand-in for the model: records what it was sent, replies with whatever the
// test told it to.
class FakeModelServer {
public:
    FakeModelServer() {
        server_.Post("/v1/chat/completions",
                     [this](const httplib::Request& req, httplib::Response& res) {
                         lastRequestBody_ = req.body;
                         lastAuthHeader_ = req.get_header_value("Authorization");
                         res.status = status_;
                         res.set_content(responseBody_, "application/json");
                     });
        port_ = server_.bind_to_any_port("127.0.0.1");
        thread_ = std::thread([this] { server_.listen_after_bind(); });
        server_.wait_until_ready();
    }

    ~FakeModelServer() {
        server_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    // Points LlmClient at this server for the duration of the test.
    void Install() const {
        setEnv("DEBATE_LLM_HOST", "127.0.0.1");
        setEnv("DEBATE_LLM_PORT", std::to_string(port_));
        setEnv("DEBATE_LLM_PATH", "/v1/chat/completions");
    }

    // Wraps `content` in the chat-completion envelope a real server returns.
    void ReplyWithContent(const std::string& content) {
        jsoncons::json message;
        message["role"] = "assistant";
        message["content"] = content;
        jsoncons::json choice;
        choice["message"] = message;
        jsoncons::json envelope;
        envelope["choices"] = jsoncons::json::array();
        envelope["choices"].push_back(choice);
        std::ostringstream out;
        out << envelope;
        responseBody_ = out.str();
        status_ = 200;
    }

    void ReplyWithRaw(int status, const std::string& body) {
        status_ = status;
        responseBody_ = body;
    }

    const std::string& last_request_body() const { return lastRequestBody_; }
    const std::string& last_auth_header() const { return lastAuthHeader_; }

private:
    httplib::Server server_;
    std::thread thread_;
    int port_ = 0;
    int status_ = 200;
    std::string responseBody_;
    std::string lastRequestBody_;
    std::string lastAuthHeader_;
};

// Restores every LlmClient env var afterwards so one test cannot leak its
// endpoint into the next -- the config is read per call, which is what makes
// overriding it possible at all.
class LlmClientTest : public ::testing::Test {
protected:
    void TearDown() override {
        for (const char* name : {"DEBATE_LLM_HOST", "DEBATE_LLM_PORT", "DEBATE_LLM_PATH",
                                 "DEBATE_LLM_MODEL", "DEBATE_LLM_MAX_TOKENS",
                                 "DEBATE_LLM_TIMEOUT_SEC", "DEBATE_LLM_API_KEY"}) {
            clearEnv(name);
        }
    }
};

} // namespace

// THE REQUEST SHAPE. Any OpenAI-compatible server rejects or misreads a body
// that gets these wrong, and the failure would look like "the model is dumb"
// rather than "the request was malformed".
TEST_F(LlmClientTest, SendsAnOpenAiChatCompletionRequest) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithContent("hello back");

    const std::string reply = LlmClient::Complete("hello there");

    EXPECT_EQ(reply, "hello back") << "the assistant's text, not the envelope";

    const jsoncons::json sent = jsoncons::json::parse(server.last_request_body());
    EXPECT_EQ(sent["model"].as<std::string>(), "qwen3.8-27b")
        << "defaults to the model server that is normally up (Qwen 3.8)";
    EXPECT_EQ(sent["max_tokens"].as<int>(), 4000);
    // Qwen 3.8 thinks by default and its thinking counts against max_tokens,
    // so a reply could come back empty without this.
    ASSERT_TRUE(sent.contains("chat_template_kwargs"));
    EXPECT_FALSE(sent["chat_template_kwargs"]["enable_thinking"].as<bool>());
    ASSERT_EQ(sent["messages"].size(), 1u);
    EXPECT_EQ(sent["messages"][0]["role"].as<std::string>(), "user");
    EXPECT_EQ(sent["messages"][0]["content"].as<std::string>(), "hello there");
    EXPECT_FALSE(sent.contains("temperature"))
        << "unset temperature must be OMITTED so the server keeps its own default";
}

// Multi-turn is what summarizing and debate-drafting will need: a system
// message setting the role, then the material.
TEST_F(LlmClientTest, ChatSendsEveryMessageInOrder) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithContent("ok");

    LlmClient::Chat({{"system", "You summarize debates."},
                     {"user", "Summarize this."},
                     {"assistant", "Sure."},
                     {"user", "Go on."}});

    const jsoncons::json sent = jsoncons::json::parse(server.last_request_body());
    ASSERT_EQ(sent["messages"].size(), 4u);
    EXPECT_EQ(sent["messages"][0]["role"].as<std::string>(), "system");
    EXPECT_EQ(sent["messages"][0]["content"].as<std::string>(), "You summarize debates.");
    EXPECT_EQ(sent["messages"][3]["content"].as<std::string>(), "Go on.");
}

TEST_F(LlmClientTest, OptionsOverrideModelTokensAndTemperature) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithContent("ok");

    LlmOptions options;
    options.model = "some-other-model";
    options.maxTokens = 128;
    options.temperature = 0.0;
    LlmClient::Complete("x", options);

    const jsoncons::json sent = jsoncons::json::parse(server.last_request_body());
    EXPECT_EQ(sent["model"].as<std::string>(), "some-other-model");
    EXPECT_EQ(sent["max_tokens"].as<int>(), 128);
    ASSERT_TRUE(sent.contains("temperature"))
        << "temperature 0 is a real setting, not an absent one";
    EXPECT_DOUBLE_EQ(sent["temperature"].as<double>(), 0.0);
}

// The env override exists so a hosted provider can be dropped in later without
// touching a single caller.
TEST_F(LlmClientTest, EnvOverridesModelAndSendsApiKeyWhenSet) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithContent("ok");
    setEnv("DEBATE_LLM_MODEL", "env-model");
    setEnv("DEBATE_LLM_API_KEY", "sk-test-123");

    LlmClient::Complete("x");

    const jsoncons::json sent = jsoncons::json::parse(server.last_request_body());
    EXPECT_EQ(sent["model"].as<std::string>(), "env-model");
    EXPECT_EQ(server.last_auth_header(), "Bearer sk-test-123");
}

TEST_F(LlmClientTest, NoAuthHeaderWhenNoApiKeyIsConfigured) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithContent("ok");

    LlmClient::Complete("x");

    EXPECT_TRUE(server.last_auth_header().empty())
        << "a local model needs no key; sending an empty Bearer would be worse than none";
}

// Models wrap JSON in a fence constantly, however firmly the prompt says not
// to. Every JSON-shaped feature would break on it, so it is handled once here.
TEST_F(LlmClientTest, CompleteJsonStripsACodeFence) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithContent("```json\n{\"core\": \"thesis\"}\n```");

    const std::string json = LlmClient::CompleteJson("extract");

    const jsoncons::json parsed = jsoncons::json::parse(json);
    EXPECT_EQ(parsed["core"].as<std::string>(), "thesis");
}

TEST_F(LlmClientTest, CompleteJsonAcceptsBareJson) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithContent("{\"core\": \"thesis\"}");

    const jsoncons::json parsed = jsoncons::json::parse(LlmClient::CompleteJson("extract"));
    EXPECT_EQ(parsed["core"].as<std::string>(), "thesis");
}

// When a model refuses, it refuses in prose. Quoting it back is the difference
// between a usable error and "parse failed".
TEST_F(LlmClientTest, CompleteJsonReportsTheModelsOwnWordsWhenItAnswersInProse) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithContent("I'm sorry, I can't help with that article.");

    try {
        LlmClient::CompleteJson("extract");
        FAIL() << "prose is not JSON and must not be returned as if it were";
    } catch (const std::runtime_error& e) {
        const std::string what = e.what();
        EXPECT_NE(what.find("did not return valid JSON"), std::string::npos);
        EXPECT_NE(what.find("I'm sorry"), std::string::npos) << "quotes the model, not just the failure";
    }
}

// The label is how a slow or failing call gets traced to the feature that made
// it once several features share this client.
TEST_F(LlmClientTest, ErrorsNameTheFeatureThatMadeTheCall) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithRaw(500, "model exploded");

    LlmOptions options;
    options.label = "debate-summary";

    try {
        LlmClient::Complete("x", options);
        FAIL() << "HTTP 500 must not be treated as a reply";
    } catch (const std::runtime_error& e) {
        const std::string what = e.what();
        EXPECT_NE(what.find("debate-summary"), std::string::npos);
        EXPECT_NE(what.find("500"), std::string::npos);
        EXPECT_NE(what.find("model exploded"), std::string::npos) << "the server's body explains why";
    }
}

TEST_F(LlmClientTest, MalformedEnvelopeIsRejectedRatherThanCrashing) {
    FakeModelServer server;
    server.Install();
    server.ReplyWithRaw(200, "{\"unexpected\": true}");

    EXPECT_THROW(LlmClient::Complete("x"), std::runtime_error)
        << "a 200 with no 'choices' is still a failed call";
}

// The model host being switched off is the ordinary case, not an exotic one.
TEST_F(LlmClientTest, UnreachableModelSaysSoAndNamesTheEndpoint) {
    setEnv("DEBATE_LLM_HOST", "127.0.0.1");
    setEnv("DEBATE_LLM_PORT", "1");  // nothing listens here

    try {
        LlmClient::Complete("x");
        FAIL() << "there is no server on port 1";
    } catch (const std::runtime_error& e) {
        const std::string what = e.what();
        EXPECT_NE(what.find("could not reach"), std::string::npos);
        EXPECT_NE(what.find("127.0.0.1:1"), std::string::npos)
            << "the endpoint is the first thing to check, so the error must state it";
    }
}

TEST_F(LlmClientTest, RefusesToSendAnEmptyConversation) {
    EXPECT_THROW(LlmClient::Chat({}), std::runtime_error);
}

TEST_F(LlmClientTest, DescribeEndpointReflectsConfiguration) {
    setEnv("DEBATE_LLM_HOST", "127.0.0.1");
    setEnv("DEBATE_LLM_PORT", "9999");
    setEnv("DEBATE_LLM_MODEL", "some-model");

    const std::string described = LlmClient::DescribeEndpoint();
    EXPECT_NE(described.find("127.0.0.1:9999"), std::string::npos);
    EXPECT_NE(described.find("some-model"), std::string::npos);
}

// A scheme-qualified host carries its own port, so appending one would produce
// a nonsense URL like "https://api.example.com:8002".
TEST_F(LlmClientTest, SchemeQualifiedHostDoesNotGetAPortAppended) {
    setEnv("DEBATE_LLM_HOST", "https://api.example.com");
    setEnv("DEBATE_LLM_PORT", "8002");

    EXPECT_EQ(LlmClient::DescribeEndpoint().find("8002"), std::string::npos);
}
