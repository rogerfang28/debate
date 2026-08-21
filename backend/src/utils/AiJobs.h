#pragma once

#include <string>

// In-flight AI work, keyed by user and kind.
//
// The model takes tens of seconds to minutes on a real debate. Doing that on
// the request thread means the page does not come back until it finishes, so
// the user gets a frozen tab and no way to tell a working model from a hung
// one. The request thread therefore starts the job and returns immediately;
// the page renders "summarizing", and later requests collect the result.
//
// State lives in memory rather than the database on purpose. A run is tied to
// one server process -- if the server restarts mid-run the work is gone and
// there is nothing to resume -- and a row saying "pending" that no thread is
// working on would be a lie that survives restarts. The finished text IS
// persisted, by the request thread that collects it.
class AiJobs {
public:
    // Which summary a run belongs to. Keyed separately so summarizing a claim
    // never cancels or collides with a whole-debate run already going.
    enum class Kind {
        Debate,      // summarize a whole debate
        Claim,       // summarize one claim
        Generation,  // build a debate from a passage
        Challenge,   // propose challenges against a debate
    };

    struct Snapshot {
        bool running = false;
        int elapsedSeconds = 0;
        // Set only when a finished job is collected; collecting clears it, so
        // exactly one caller ever sees a given result.
        bool finished = false;
        std::string text;
        std::string error;
        // Move-log head when the source text was taken. Carried on the JOB
        // because the run outlives any single response: the page is rebuilt
        // several times while a summary generates, so anything staged on the
        // response is gone by the time the result comes back.
        int atMoveSeq = 0;
        // Which claim a claim-summary run is about. On the job for the same
        // reason as atMoveSeq: the viewer can select a different claim while
        // this one generates, so "the claim being looked at" is not the same
        // question as "the claim this result belongs to".
        int claimId = 0;
    };

    // Starts a run for this user. A second call while one is running is
    // ignored -- double-clicking Summarize must not fire two model calls.
    // Returns false if a run was already in progress.
    static bool Start(int userId, Kind kind, const std::string& debateText, int atMoveSeq,
                      int claimId = 0);

    // Reports whether a run is going, and hands over the result exactly once.
    static Snapshot Collect(int userId, Kind kind);

    // True if a run is in flight, without consuming anything.
    static bool IsRunning(int userId, Kind kind);

    // Stops WAITING for a run. The worker thread cannot be interrupted -- it is
    // blocked reading from a model that will reply in its own time -- so this
    // marks the job abandoned and its result is dropped when it arrives. The
    // user gets their page back immediately, which is the thing they wanted.
    static void Abandon(int userId, Kind kind);
};
