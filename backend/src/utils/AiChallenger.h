#pragma once

#include <string>

class DebateModerator;

// Asks the model to attack a debate, and places the attacks it can.
//
// Deliberately adversarial: the summary describes a debate, this one tries to
// break it. Every challenge names the claim it targets by the id the rendered
// debate already shows, so it lands on a specific sentence rather than
// floating next to the argument.
class AiChallenger {
public:
    struct Result {
        bool ok = false;
        int challengesAdded = 0;
        // Challenges naming a claim that is not in this debate. Counted rather
        // than guessed at: placing one on the wrong claim would put words into
        // somebody's argument.
        int skipped = 0;
        std::string error;
    };

    // `debateText` is DebateTextFormatter::Format output -- the same rendering
    // the summary reads, with its [#id] markers.
    static std::string BuildPrompt(const std::string& debateText);

    // Applies the model's reply through the moderator, so the challenges are
    // real moves, indistinguishable from typed ones.
    static Result Apply(const std::string& json, int user_id, DebateModerator& moderator);
};
