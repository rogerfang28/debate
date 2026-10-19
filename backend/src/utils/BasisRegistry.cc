#include "BasisRegistry.h"

#include "Log.h"
#include "pathUtils.h"
#include "google/protobuf/text_format.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>

namespace {

// Placeholder strings the pre-basis code wrote into the link's `connection`
// column. They are not bases and were never chosen by anyone, so they get the
// legacy behaviour without a warning -- warning on every old link would drown
// the log in the history of the schema.
bool isLegacyPlaceholder(const std::string& name) {
    return name == "parent to child connection" || name == "challenge link";
}

struct Cache {
    std::mutex mutex;
    std::string path;
    std::filesystem::file_time_type mtime{};
    std::uintmax_t size = 0;
    bool loaded = false;
    bool reportedMissing = false;
    basis::BasisRegistry registry;
};

Cache& cache() {
    static Cache c;
    return c;
}

bool readFile(const std::string& path, std::string& out) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    out = contents.str();
    return true;
}

// Re-parses when the path, modification time or size has changed since the
// last parse. Size is compared as well as mtime because a write that lands
// within the filesystem's timestamp granularity would otherwise be missed.
void refreshLocked(Cache& c) {
    const std::string path = bases::FilePath();

    std::error_code ec;
    const bool exists = !path.empty() && std::filesystem::is_regular_file(path, ec);
    if (!exists) {
        if (!c.reportedMissing || c.path != path) {
            Log::error("[Bases] No basis registry at '" + path +
                       "'. Set DEBATE_BASES_FILE or add backend/config/bases.pbtxt. "
                       "Running with no bases: dropdowns will be empty and every link "
                       "propagates as COLLAPSE.");
            c.reportedMissing = true;
        }
        c.path = path;
        c.loaded = true;
        c.registry.Clear();
        return;
    }

    const auto mtime = std::filesystem::last_write_time(path, ec);
    const auto size = std::filesystem::file_size(path, ec);
    if (c.loaded && c.path == path && c.mtime == mtime && c.size == size) {
        return;  // unchanged
    }

    std::string text;
    if (!readFile(path, text)) {
        Log::error("[Bases] Could not open " + path);
        c.registry.Clear();
    } else {
        basis::BasisRegistry parsed;
        if (!google::protobuf::TextFormat::ParseFromString(text, &parsed)) {
            // A parse failure keeps whatever was loaded before, if anything:
            // a half-saved edit should not blank the dropdowns of a running
            // server. It is logged every time the broken file is seen changed.
            Log::error("[Bases] " + path + " is not valid protobuf text format; keeping the "
                       "previously loaded registry (" +
                       std::to_string(c.registry.bases_size()) + " bases).");
        } else {
            c.registry = parsed;
            Log::info("[Bases] Loaded " + std::to_string(parsed.bases_size()) + " bases from " +
                      path);
        }
    }
    c.path = path;
    c.mtime = mtime;
    c.size = size;
    c.loaded = true;
    c.reportedMissing = false;
}

const basis::Basis* find(const basis::BasisRegistry& registry, const std::string& name) {
    for (const auto& b : registry.bases()) {
        if (b.name() == name) {
            return &b;
        }
    }
    return nullptr;
}

} // namespace

namespace bases {

std::string FilePath() {
    if (const char* configured = std::getenv("DEBATE_BASES_FILE");
        configured != nullptr && configured[0] != '\0') {
        std::filesystem::path p(configured);
        if (p.is_relative()) {
            p = std::filesystem::current_path() / p;
        }
        return p.lexically_normal().string();
    }

    // The server binary lives in backend/build/bin; the test and tool
    // binaries in test/build. Each is a different number of steps from
    // backend/config, so try the ones that could be right and take the first
    // that exists. The last candidate is returned even if absent, so the
    // error message names a real place to put the file.
    const std::filesystem::path exeDir = utils::getExeDir();
    const std::filesystem::path candidates[] = {
        exeDir / ".." / ".." / "config" / "bases.pbtxt",              // backend/build/bin
        exeDir / ".." / ".." / "backend" / "config" / "bases.pbtxt",  // test/build
        exeDir / ".." / "backend" / "config" / "bases.pbtxt",
        exeDir / "config" / "bases.pbtxt",
    };
    std::error_code ec;
    for (const auto& candidate : candidates) {
        const auto normal = std::filesystem::weakly_canonical(candidate, ec);
        if (!ec && std::filesystem::is_regular_file(normal, ec)) {
            return normal.string();
        }
    }
    return std::filesystem::weakly_canonical(candidates[0], ec).string();
}

const basis::BasisRegistry& Registry() {
    Cache& c = cache();
    std::lock_guard<std::mutex> lock(c.mutex);
    refreshLocked(c);
    return c.registry;
}

std::vector<std::string> Names() {
    std::vector<std::string> names;
    for (const auto& b : Registry().bases()) {
        if (!b.name().empty()) {
            names.push_back(b.name());
        }
    }
    return names;
}

std::string DefaultName() {
    const basis::BasisRegistry& registry = Registry();
    for (const auto& b : registry.bases()) {
        if (b.is_default() && !b.name().empty()) {
            return b.name();
        }
    }
    for (const auto& b : registry.bases()) {
        if (!b.name().empty()) {
            return b.name();
        }
    }
    return "";
}

bool IsKnown(const std::string& name) {
    return !name.empty() && find(Registry(), name) != nullptr;
}

std::string Sanitize(const std::string& name) {
    if (name.empty()) {
        return "";
    }
    if (IsKnown(name)) {
        return name;
    }
    Log::warn("[Bases] Rejected unknown basis '" + name + "'; storing the link with no basis.");
    return "";
}

basis::Propagation PropagationFor(const std::string& name) {
    if (name.empty() || isLegacyPlaceholder(name)) {
        return basis::COLLAPSE;
    }
    const basis::Basis* b = find(Registry(), name);
    if (b == nullptr || b->propagation() == basis::PROPAGATION_UNSPECIFIED) {
        return basis::COLLAPSE;
    }
    return b->propagation();
}

std::string GuideAsList() {
    const basis::BasisRegistry& registry = Registry();
    if (registry.bases_size() == 0) {
        return "   (no kinds of reason are configured -- omit the basis line)";
    }
    std::ostringstream out;
    for (const auto& b : registry.bases()) {
        out << "   " << b.name() << " -- " << b.description();
        if (!b.as_support().empty()) {
            out << " Supporting: " << b.as_support();
        }
        if (!b.as_challenge().empty()) {
            out << " Challenging: " << b.as_challenge();
        }
        out << "\n";
    }
    return out.str();
}

std::string GuideInline() {
    const basis::BasisRegistry& registry = Registry();
    if (registry.bases_size() == 0) {
        return "(none configured)";
    }
    std::ostringstream out;
    bool first = true;
    for (const auto& b : registry.bases()) {
        if (!first) {
            out << ", ";
        }
        first = false;
        out << b.name();
        const std::string& gloss = b.as_challenge().empty() ? b.description() : b.as_challenge();
        if (!gloss.empty()) {
            out << " (" << gloss << ")";
        }
    }
    return out.str();
}

void ResetCache() {
    Cache& c = cache();
    std::lock_guard<std::mutex> lock(c.mutex);
    c.loaded = false;
    c.reportedMissing = false;
    c.path.clear();
    c.registry.Clear();
}

} // namespace bases
