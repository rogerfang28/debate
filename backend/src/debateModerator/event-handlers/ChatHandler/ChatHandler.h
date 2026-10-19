#pragma once

#include <string>
#include "../../../utils/DebateWrapper.h"

// The AI chat: the one place the model speaks in this tool.
//
// A reader asks a question about the debate they are in -- typed, or one
// of the quick prompts -- and the model answers with text. Nothing it says
// becomes a claim; the reader decides what to do with it. Each reader has
// their own conversation per debate, kept in CHATS and mirrored onto the
// user record so the page can draw it without a second lookup.
//
// The model takes tens of seconds, so Ask hands the question to AiJobs and
// returns; Refresh, run on every response, folds progress and the answer
// back in. That is what the page polls on.
class ChatHandler {
public:
    // `quickPrompt` is a DebateChat key, or "" for a typed `question`.
    static void Ask(const std::string& question, const std::string& quickPrompt,
                    const int& user_id, DebateWrapper& debateWrapper);

    // Folds any running or finished answer into the user record. Also loads
    // the transcript the first time a user looks at a debate.
    static void Refresh(const int& user_id, DebateWrapper& debateWrapper);

    // Forgets the conversation for the debate the user is in.
    static void Clear(const int& user_id, DebateWrapper& debateWrapper);
};
