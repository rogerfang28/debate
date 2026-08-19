// Iterate on the summary PROMPT against a real debate without rebuilding.
//
// The prompt is loaded from a file at runtime, so tuning wording is an edit and
// a re-run rather than an edit, a rebuild and a click through the UI. Once the
// wording is right it gets baked into DebateSummarizer.
//
//   summarize_tool <debate_id> [prompt_file] [debates_db] [users_db]
//
// Prints the flattened debate to stderr (so it can be inspected separately) and
// the model's summary to stdout.

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "database/sqlite/Database.h"
#include "debateModerator/DebateModerator.h"
#include "debateModerator/buildResponse/debatePageResponse/BuildCollection.h"
#include "utils/DebateSummarizer.h"
#include "utils/DebateTextFormatter.h"
#include "utils/DebateWrapper.h"
#include "utils/LlmClient.h"
#include "utils/Log.h"
#include "utils/pathUtils.h"

namespace {

std::string ReadFile(const std::string& path, bool& ok) {
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        ok = false;
        return "";
    }
    std::ostringstream buffer;
    buffer << in.rdbuf();
    ok = true;
    return buffer.str();
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: summarize_tool <debate_id> [prompt_file] [debates_db] [users_db]\n";
        return 1;
    }

    const int debateId = std::atoi(argv[1]);
    const std::string promptFile = (argc >= 3) ? argv[2] : "";
    if (argc >= 4) {
        _putenv_s("DB_PATH", argv[3]);
    }
    const std::string usersDbPath = (argc >= 5) ? argv[4] : utils::getDatabasePath();

    Log::init(LogLevel::Warn);

    Database usersDb(usersDbPath);
    DebateModerator moderator(usersDb);
    DebateWrapper& debateWrapper = moderator.getDebateWrapper();

    const std::vector<int> memberIds = debateWrapper.findUsersInDebate(debateId);
    const debate::Collection collection =
        BuildCollection::BuildForDebateAndUsers(debateId, memberIds, debateWrapper);
    if (collection.claims_by_id().empty()) {
        std::cerr << "debate " << debateId << " has no claims\n";
        return 1;
    }

    std::map<int, std::string> usernames;
    for (int memberId : memberIds) {
        usernames[memberId] = debateWrapper.getUserProtobuf(memberId).username();
    }
    for (const auto& entry : collection.claims_by_id()) {
        const int creatorId = entry.second.creator_id();
        if (creatorId > 0 && usernames.find(creatorId) == usernames.end()) {
            usernames[creatorId] = debateWrapper.getUserProtobuf(creatorId).username();
        }
    }

    std::string topic;
    const std::vector<uint8_t> debateBytes = debateWrapper.getDebateProtobuf(debateId);
    debate::Debate debateProto;
    if (!debateBytes.empty() &&
        debateProto.ParseFromArray(debateBytes.data(), static_cast<int>(debateBytes.size()))) {
        topic = debateProto.topic();
    }

    DebateTextFormatter::Options options;
    options.topic = topic;
    options.viewerUsername = memberIds.empty() ? "" : usernames[memberIds.front()];
    const std::string debateText = DebateTextFormatter::Format(collection, usernames, options);

    std::cerr << "===== DEBATE TEXT (" << debateText.size() << " chars) =====\n"
              << debateText << "\n===== END DEBATE TEXT =====\n";

    std::string summary;
    if (promptFile.empty()) {
        summary = DebateSummarizer::Summarize(debateText);
    } else {
        bool ok = false;
        std::string prompt = ReadFile(promptFile, ok);
        if (!ok) {
            std::cerr << "could not read prompt file: " << promptFile << "\n";
            return 1;
        }
        const std::string placeholder = "{debate}";
        const size_t pos = prompt.find(placeholder);
        if (pos != std::string::npos) {
            prompt.replace(pos, placeholder.size(), debateText);
        }
        LlmOptions llmOptions;
        llmOptions.label = "summary-tuning";
        llmOptions.temperature = 0.2;
        summary = LlmClient::Complete(prompt, llmOptions);
    }

    std::cout << summary << std::endl;
    return 0;
}
