#pragma once

#include <string>

class DebateModerator;
class DebateWrapper;

// The AI features that WRITE to a debate, as opposed to the ones that read it.
//
// Both run the model on a background thread and build on the request thread,
// for the same reason the summaries do: the model takes a minute or more, and
// only the request thread may touch the database.
class AiAuthorHandler {
public:
    // Builds a whole new debate from a passage. The topic steers what the root
    // claim should be about; an empty topic lets the passage decide.
    static void GenerateDebate(const std::string& topic, const std::string& passage,
                               const int& user_id, DebateWrapper& debateWrapper);

    // Proposes challenges against the debate the user is in, and adds the ones
    // it can place. Attributed to the user, like anything else they cause.
    static void ChallengeWithAi(const int& user_id, DebateWrapper& debateWrapper);

    // Applies a finished generation or challenge run. Called on every response
    // build, like the summary equivalent -- this is where the model's JSON
    // becomes claims.
    static void RefreshAiWork(const int& user_id, DebateModerator& moderator);

    // Clears the "what happened" notice once the user has read it.
    static void DismissNotice(const int& user_id, DebateWrapper& debateWrapper);

    // Stops waiting on whatever AI work is running for this user.
    static void CancelWork(const int& user_id, DebateWrapper& debateWrapper);
};
