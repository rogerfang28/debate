#pragma once

#include <string>

// The words the AI chat is given: how to read a debate, what it may and may
// not do, and the canned questions behind the quick buttons.
//
// Everything the model does in this tool happens here, as text. It never
// writes a claim; the reader does that. So the rules are about honesty --
// add nothing, invent no critic, decide nothing the reader did not ask it
// to decide -- and the quick prompts are just questions a reader asks a lot.
namespace DebateChat {

// Quick-prompt keys, as they appear in button ids (chatQuickButton_<key>).
constexpr const char* kSummarizeDebate = "summarize_debate";
constexpr const char* kSummarizeClaim = "summarize_claim";
constexpr const char* kFindMistake = "find_mistake";
constexpr const char* kSuggestChallenge = "suggest_challenge";

// The system message: rules plus the rendered debate.
std::string SystemPrompt(const std::string& debateText, const std::string& viewerName);

// The question a quick prompt stands for. Claim prompts need the selected
// claim; with none selected they return "" and the caller does nothing.
// An unknown key returns "".
std::string QuestionFor(const std::string& quickPrompt, int claimId, const std::string& claimSentence);

// The label shown on the quick button for a key, or "" for an unknown key.
std::string LabelFor(const std::string& quickPrompt);

}  // namespace DebateChat
