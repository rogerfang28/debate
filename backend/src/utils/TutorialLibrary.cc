#include "TutorialLibrary.h"

#include "Log.h"
#include "pathUtils.h"
#include "google/protobuf/text_format.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace {

constexpr const char* kExtension = ".pbtxt";

std::string readWholeFile(const std::filesystem::path& path, bool& ok) {
    std::ifstream file(path);
    if (!file.is_open()) {
        ok = false;
        return {};
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    ok = true;
    return contents.str();
}

} // namespace

namespace tutorials {

std::string Directory() {
    if (const char* configured = std::getenv("DEBATE_TUTORIALS_DIR");
        configured != nullptr && configured[0] != '\0') {
        std::filesystem::path dir(configured);
        if (dir.is_relative()) {
            dir = std::filesystem::current_path() / dir;
        }
        return dir.lexically_normal().string();
    }
    // The executable lives in backend/build/bin/, so the scripts are two levels
    // up. Same walk getDatabasePath does, for the same reason: the working
    // directory a server is started from is not something to rely on.
    std::filesystem::path dir = utils::getExeDir() / ".." / ".." / "tutorials";
    return std::filesystem::weakly_canonical(dir).string();
}

std::vector<Listing> List() {
    std::vector<Listing> found;
    std::error_code ec;
    const std::filesystem::path dir(Directory());
    if (!std::filesystem::is_directory(dir, ec)) {
        Log::debug("[Tutorials] No tutorials directory at " + dir.string());
        return found;
    }

    for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
        if (!entry.is_regular_file() || entry.path().extension() != kExtension) {
            continue;
        }
        const std::string name = entry.path().stem().string();
        bool ok = false;
        std::string error;
        const tutorial::Tutorial loaded = Load(name, ok, error);
        if (!ok) {
            // Skipped, not fatal. A half-written script in the directory should
            // cost you that one walkthrough, not the whole home page.
            Log::warn("[Tutorials] Skipping " + entry.path().filename().string() + ": " + error);
            continue;
        }
        Listing listing;
        listing.name = name;
        listing.title = loaded.title().empty() ? name : loaded.title();
        listing.subtitle = loaded.subtitle();
        found.push_back(listing);
    }

    std::sort(found.begin(), found.end(),
              [](const Listing& a, const Listing& b) { return a.name < b.name; });
    return found;
}

std::string DefaultName() {
    if (const char* configured = std::getenv("DEBATE_TUTORIAL");
        configured != nullptr && configured[0] != '\0') {
        return std::string(configured);
    }
    const std::vector<Listing> all = List();
    return all.empty() ? std::string() : all.front().name;
}

tutorial::Tutorial Load(const std::string& name, bool& ok, std::string& error) {
    tutorial::Tutorial loaded;
    ok = false;

    if (name.empty()) {
        error = "No tutorial was named.";
        return loaded;
    }
    // A name reaches this from a client message, so it decides which file gets
    // opened. Anything that could climb out of the tutorials directory is
    // refused rather than sanitised, because a rewritten path that still opens
    // SOMETHING is harder to notice than a refusal.
    if (name.find('/') != std::string::npos || name.find('\\') != std::string::npos ||
        name.find("..") != std::string::npos) {
        error = "That tutorial name is not allowed.";
        return loaded;
    }

    const std::filesystem::path path = std::filesystem::path(Directory()) / (name + kExtension);
    bool read = false;
    const std::string text = readWholeFile(path, read);
    if (!read) {
        error = "Could not open the tutorial script " + name + kExtension + ".";
        return loaded;
    }
    if (!google::protobuf::TextFormat::ParseFromString(text, &loaded)) {
        error = "The tutorial script " + name + kExtension +
                " is not valid protobuf text format.";
        return loaded;
    }
    if (loaded.steps_size() == 0) {
        error = "The tutorial script " + name + kExtension + " has no steps.";
        return loaded;
    }

    if (loaded.name().empty()) {
        loaded.set_name(name);
    }
    ok = true;
    return loaded;
}

} // namespace tutorials
