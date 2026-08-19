#include "SummaryJobs.h"

#include "DebateSummarizer.h"
#include "Log.h"

#include <chrono>
#include <exception>
#include <map>
#include <mutex>
#include <thread>

namespace {

struct Job {
    bool running = false;
    bool finished = false;
    std::chrono::steady_clock::time_point startedAt;
    std::string text;
    std::string error;
};

std::mutex& mutex() {
    static std::mutex m;
    return m;
}

std::map<int, Job>& jobs() {
    static std::map<int, Job> j;
    return j;
}

} // namespace

bool SummaryJobs::Start(int userId, const std::string& debateText) {
    {
        std::lock_guard<std::mutex> lock(mutex());
        Job& job = jobs()[userId];
        if (job.running) {
            Log::info("[SummaryJobs] user " + std::to_string(userId) +
                      " already has a summary running; ignoring the second request");
            return false;
        }
        job = Job{};
        job.running = true;
        job.startedAt = std::chrono::steady_clock::now();
    }

    // Detached: nothing joins this, and the result is picked up through the
    // registry rather than by waiting on the thread.
    std::thread([userId, debateText]() {
        std::string text;
        std::string error;
        try {
            text = DebateSummarizer::Summarize(debateText);
            if (text.empty()) {
                error = "The model returned an empty summary.";
            }
        } catch (const std::exception& e) {
            // Caught here rather than allowed to escape: an exception leaving a
            // detached thread terminates the whole server, so a model that is
            // merely down would take the site with it.
            Log::warn(std::string("[SummaryJobs] summary failed: ") + e.what());
            error = e.what();
        } catch (...) {
            error = "The summary failed for an unknown reason.";
        }

        std::lock_guard<std::mutex> lock(mutex());
        Job& job = jobs()[userId];
        job.running = false;
        job.finished = true;
        job.text = text;
        job.error = error;
    }).detach();

    return true;
}

SummaryJobs::Snapshot SummaryJobs::Collect(int userId) {
    std::lock_guard<std::mutex> lock(mutex());
    auto it = jobs().find(userId);
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
        // Handed over once. Leaving it would re-apply the same summary on every
        // later page build, silently overwriting a newer one.
        jobs().erase(it);
    }
    return snapshot;
}

bool SummaryJobs::IsRunning(int userId) {
    std::lock_guard<std::mutex> lock(mutex());
    auto it = jobs().find(userId);
    return it != jobs().end() && it->second.running;
}
