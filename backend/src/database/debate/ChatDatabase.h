#pragma once

#include <string>
#include <vector>
#include "../sqlite/Database.h"

// ---------------------------------------
// ChatDatabase
// ---------------------------------------
// The AI chat transcript: one row per message, per user, per debate.
//
// A chat is a conversation, so it is a log, not a slot. Each user has their
// own conversation about a debate -- what one person asked is not something
// another person is shown. Rows are appended in order and read back in
// order; nothing is ever updated.
// ---------------------------------------
class ChatDatabase {
public:
    explicit ChatDatabase(Database& db);

    bool ensureTable();

    struct Message {
        std::string role;  // "user" or "assistant"
        std::string text;
    };

    // Appends one message. Refuses a message with no user or debate.
    bool append(int userId, int debateId, const std::string& role, const std::string& text);

    // Every message of one user's conversation about one debate, oldest first.
    std::vector<Message> list(int userId, int debateId);

    // Forgets one user's conversation about one debate.
    bool clear(int userId, int debateId);

private:
    Database& db_;
};
