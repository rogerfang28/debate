#include "AiJobs.h"

#include "DebateSummarizer.h"
#include "LlmClient.h"
#include "Log.h"

#include <chrono>
#include <exception>
#include <map>
#include <utility>
#include <mutex>
#include <thread>

namespace {

struct Job {
    bool running = false;
    bool finished = false;
    std::chrono::steady_clock::time_point startedAt;
    int atMoveSeq = 0;
    int claimId = 0;
    std::string text;
    std::string error;
};

LlmOptions promptOptions(AiJobs::Kind kind) {
    LlmOptions options;
    options.label = kind == AiJobs::Kind::Generation ? "debate-generation" : "ai-challenge";
    // Both are extraction from a given text, not invention. Latitude here is
    // exactly how a made-up claim gets into somebody's debate.
    options.temperature = 0.1;
    return options;
}

std::mutex& mutex() {
    static std::mutex m;
    return m;
}

using JobKey = std::pair<int, AiJobs::Kind>;

std::map<JobKey, Job>& jobs() {
    static std::map<JobKey, Job> j;
    return j;
}

} // namespace

bool AiJobs::Start(int userId, Kind kind, const std::string& debateText, int atMoveSeq,
                        int claimId) {
    const JobKey key{userId, kind};
    {
        std::lock_guard<std::mutex> lock(mutex());
        Job& job = jobs()[key];
        if (job.running) {
            Log::info("[AiJobs] user " + std::to_string(userId) +
                      " already has a summary running; ignoring the second request");
            return false;
        }
        job = Job{};
        job.running = true;
        job.startedAt = std::chrono::steady_clock::now();
        job.atMoveSeq = atMoveSeq;
        job.claimId = claimId;
    }

    // Detached: nothing joins this, and the result is picked up through the
    // registry rather than by waiting on the thread.
    std::thread([key, kind, debateText]() {
        std::string text;
        std::string error;
        try {
            switch (kind) {
                case Kind::Claim:      text = DebateSummarizer::SummarizeClaim(debateText); break;
                case Kind::Debate:     text = DebateSummarizer::Summarize(debateText); break;
                // Generation and Challenge carry their own prompts, so the
                // caller hands over a finished prompt and this just runs it.
                // Their results are JSON that a request thread turns into
                // claims -- the worker never touches the database.
                // Challenge answers in JSON; generation answers with a
                // scenario file, which is not JSON and must not be validated
                // as though it were.
                case Kind::Challenge:  text = LlmClient::CompleteJson(debateText, promptOptions(kind)); break;
                case Kind::Generation: text = LlmClient::Complete(debateText, promptOptions(kind)); break;
            }
            if (text.empty()) {
                error = "The model returned an empty summary.";
            }
        } catch (const std::exception& e) {
            // Caught here rather than allowed to escape: an exception leaving a
            // detached thread terminates the whole server, so a model that is
            // merely down would take the site with it.
            Log::warn(std::string("[AiJobs] summary failed: ") + e.what());
            error = e.what();
        } catch (...) {
            error = "The summary failed for an unknown reason.";
        }

        std::lock_guard<std::mutex> lock(mutex());
        Job& job = jobs()[key];
        job.running = false;
        job.finished = true;
        job.text = text;
        job.error = error;
    }).detach();

    return true;
}

AiJobs::Snapshot AiJobs::Collect(int userId, Kind kind) {
    std::lock_guard<std::mutex> lock(mutex());
    auto it = jobs().find(JobKey{userId, kind});
    if (it == jobs().end()) {
        return Snapshot{};
    }

    Snapshot snapshot;
    Job& job = it->second;
    snapshot.running = job.running;
    if (job.running) {
        snapshot.elapsedSeconds = static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - job.startedAt).count());
        return snapshot;
    }

    if (job.finished) {
        snapshot.finished = true;
        snapshot.text = job.text;
        snapshot.error = job.error;
        snapshot.atMoveSeq = job.atMoveSeq;
        snapshot.claimId = job.claimId;
        // Handed over once. Leaving it would re-apply the same summary on every
        // later page build, silently overwriting a newer one.
        jobs().erase(it);
    }
    return snapshot;
}

bool AiJobs::IsRunning(int userId, Kind kind) {
    std::lock_guard<std::mutex> lock(mutex());
    auto it = jobs().find(JobKey{userId, kind});
    return it != jobs().end() && it->second.running;
}
