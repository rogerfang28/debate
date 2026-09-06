#include "ExtractedDebateBuilder.h"

#include "BasisRegistry.h"
#include "DebateWrapper.h"
#include "Log.h"
#include "debate_event.pb.h"
#include "google/protobuf/text_format.h"
#include "google/protobuf/io/tokenizer.h"

#include <algorithm>
#include <set>
#include <string>
#include <vector>

namespace {

// A tree deeper or wider than this is not an argument, it is a runaway. The
// caps exist so a bad generation is refused rather than spending minutes
// writing thousands of rows.
constexpr int kMaxClaims = 200;
constexpr int kMaxDepth = 15;

// Models wrap output in a fence however firmly they are told not to.
std::string stripCodeFence(const std::string& text) {
    const size_t start = text.find("```");
    if (start == std::string::npos) {
        return text;
    }
    size_t contentStart = text.find('\n', start);
    if (contentStart == std::string::npos) {
        return text;
    }
    contentStart += 1;
    const size_t end = text.find("```", contentStart);
    if (end == std::string::npos) {
        return text.substr(contentStart);
    }
    return text.substr(contentStart, end - contentStart);
}

// Captures the parser's own complaint. Without this a malformed tree came back
// as "not valid protobuf text format" and nothing else -- useless to the reader
// and useless to whoever has to work out whether the PROMPT is at fault. A
// model writes this input, so the reason it failed is the most valuable thing
// the failure produces.
class ParseErrors : public google::protobuf::io::ErrorCollector {
public:
    void RecordError(int line, google::protobuf::io::ColumnNumber column,
                     absl::string_view message) override {
        if (!first_.empty()) {
            return;  // the first error is the real one; the rest are its wake
        }
        // Lines are zero-based here and one-based to everyone else.
        first_ = "line " + std::to_string(line + 1) + ", column " +
                 std::to_string(static_cast<int>(column) + 1) + ": " +
                 std::string(message);
    }
    void RecordWarning(int, google::protobuf::io::ColumnNumber, absl::string_view) override {}

    const std::string& first() const { return first_; }

private:
    std::string first_;
};

bool isBlank(const std::string& s) {
    return s.find_first_not_of(" \t\r\n") == std::string::npos;
}

debate_event::DebateEvent forge(int user_id, debate_event::EventType type) {
    debate_event::DebateEvent event;
    event.set_type(type);
    event.mutable_user()->set_user_id(user_id);
    event.mutable_user()->set_is_logged_in(true);
    return event;
}

std::set<int> claimIdsIn(int debateId, DebateWrapper& debateWrapper) {
    std::set<int> ids;
    for (const auto& blob : debateWrapper.getStatementsForDebate(debateId)) {
        debate::Claim claim;
        if (claim.ParseFromArray(blob.data(), static_cast<int>(blob.size()))) {
            ids.insert(claim.id());
        }
    }
    return ids;
}

// Walks the tree before anything is written. Counts as it goes so the caps are
// enforced on the whole tree rather than on one branch.
bool validateClaim(const extraction::Claim& claim, int depth, int& claimCount,
                   std::string& error) {
    if (depth > kMaxDepth) {
        error = "The tree is nested deeper than " + std::to_string(kMaxDepth) + " levels.";
        return false;
    }
    if (isBlank(claim.text())) {
        error = "A claim has no text.";
        return false;
    }
    if (++claimCount > kMaxClaims) {
        error = "The tree has more than " + std::to_string(kMaxClaims) + " claims.";
        return false;
    }

    for (const extraction::Argument& argument : claim.arguments()) {
        if (argument.role() == extraction::ROLE_UNSPECIFIED) {
            error = "An argument under \"" + claim.text().substr(0, 60) +
                    "\" has no role; it must be SUPPORTS or OPPOSES.";
            return false;
        }
        int usable = 0;
        for (const extraction::Claim& child : argument.claims()) {
            if (!isBlank(child.text())) {
                ++usable;
            }
        }
        if (usable == 0) {
            error = "An argument under \"" + claim.text().substr(0, 60) + "\" has no claims.";
            return false;
        }
        for (const extraction::Claim& child : argument.claims()) {
            if (isBlank(child.text())) {
                continue;  // a blank entry is dropped, not fatal
            }
            if (!validateClaim(child, depth + 1, claimCount, error)) {
                return false;
            }
        }
    }
    return true;
}

// One write: the claims of a single argument, created together under `parentId`.
struct Group {
    const extraction::Argument* argument = nullptr;
    std::vector<const extraction::Claim*> claims;
};

// Splits an argument into the groups that will actually be written.
//
// Several claims stay together only with a stated joint_reason. The model
// groups by default and groups wrongly, so with nothing stated the claims
// become independent arguments -- the conservative reading, and the opposite
// of the direction the model errs in.
std::vector<Group> groupsOf(const extraction::Argument& argument, int& groupsSplit) {
    std::vector<const extraction::Claim*> usable;
    for (const extraction::Claim& child : argument.claims()) {
        if (!isBlank(child.text())) {
            usable.push_back(&child);
        }
    }

    std::vector<Group> groups;
    if (usable.size() > 1 && isBlank(argument.joint_reason())) {
        ++groupsSplit;
        for (const extraction::Claim* claim : usable) {
            Group single;
            single.argument = &argument;
            single.claims.push_back(claim);
            groups.push_back(single);
        }
        return groups;
    }

    Group whole;
    whole.argument = &argument;
    whole.claims = usable;
    groups.push_back(whole);
    return groups;
}

} // namespace

bool ExtractedDebateBuilder::Parse(const std::string& text, extraction::ExtractedDebate& out,
                                   std::string& error) {
    ParseErrors errors;
    google::protobuf::TextFormat::Parser parser;
    parser.RecordErrorsTo(&errors);
    if (!parser.ParseFromString(stripCodeFence(text), &out)) {
        // A model wrote this, so a parse failure is an expected outcome and the
        // reason has to reach the user rather than a log nobody reads. It is
        // also the GOOD failure: a malformed tree is refused, where a
        // mis-numbered step list used to import a quietly wrong graph.
        error = "The generated tree was not valid protobuf text format";
        if (!errors.first().empty()) {
            error += " (" + errors.first() + ")";
        }
        error += ".";
        Log::warn("[ExtractedDebate] Parse failed: " + error);
        return false;
    }
    if (!out.has_root() || isBlank(out.root().text())) {
        error = "The generated tree has no root claim.";
        return false;
    }
    int claimCount = 0;
    if (!validateClaim(out.root(), 0, claimCount, error)) {
        return false;
    }
    return true;
}

ExtractedDebateBuilder::Result ExtractedDebateBuilder::Build(const std::string& text, int user_id,
                                                             DebateModerator& moderator) {
    Result result;
    extraction::ExtractedDebate tree;
    if (!Parse(text, tree, result.error)) {
        return result;
    }

    DebateWrapper& debateWrapper = moderator.getDebateWrapper();

    const std::vector<int> debatesBefore = debateWrapper.getUserDebateIds(user_id);
    {
        auto event = forge(user_id, debate_event::CREATE_DEBATE);
        event.mutable_create_debate()->set_debate_topic(tree.root().text());
        moderator.handleRequest(event);
    }

    int debateId = 0;
    for (int id : debateWrapper.getUserDebateIds(user_id)) {
        if (std::find(debatesBefore.begin(), debatesBefore.end(), id) == debatesBefore.end()) {
            debateId = id;
        }
    }
    if (debateId <= 0) {
        result.error = "The debate could not be created.";
        return result;
    }
    result.debateId = debateId;

    // AddDebate creates the debate but does not put the author inside it, and
    // the write handlers all act on where the user IS. Without this the claims
    // are attempted against debate 0 and silently nothing is created.
    {
        auto event = forge(user_id, debate_event::ENTER_DEBATE);
        event.mutable_enter_debate()->set_debate_id(debateId);
        moderator.handleRequest(event);
    }

    std::set<int> existing = claimIdsIn(debateId, debateWrapper);
    if (existing.size() != 1) {
        result.error = "The new debate did not come back with exactly one root claim.";
        return result;
    }
    const int rootId = *existing.begin();
    result.claimsCreated = 1;

    // Depth-first. The recursion carries the REAL parent id, which is the whole
    // point of a tree: there is no name map and no numbering, because the id a
    // child hangs from is the one just created for its parent.
    struct Writer {
        int user_id;
        int debateId;
        DebateModerator& moderator;
        DebateWrapper& debateWrapper;
        std::set<int>& existing;
        Result& result;

        bool write(const extraction::Claim& parent, int parentId, int depth) {
            result.maxDepthReached = std::max(result.maxDepthReached, depth);
            for (const extraction::Argument& argument : parent.arguments()) {
                for (const Group& group : groupsOf(argument, result.groupsSplit)) {
                    if (!writeGroup(group, parentId, depth)) {
                        return false;
                    }
                }
            }
            return true;
        }

        bool writeGroup(const Group& group, int parentId, int depth) {
            const extraction::Argument& argument = *group.argument;

            // Only a basis the registry knows is stored. The claims are still
            // created; a link that merely lacks a basis is far better than one
            // carrying a name nothing in the tool can explain.
            const std::string basis = bases::Sanitize(argument.basis());
            if (basis.empty() && !isBlank(argument.basis())) {
                result.unknownBases++;
            }

            // The handlers act on where the user IS, so navigating is part of
            // the write -- exactly as it is when a person clicks through the UI.
            {
                auto event = forge(user_id, debate_event::GO_TO_CLAIM);
                event.mutable_go_to_claim()->set_claim_id(parentId);
                moderator.handleRequest(event);
            }

            if (argument.role() == extraction::OPPOSES) {
                auto start = forge(user_id, debate_event::START_CHALLENGE_CLAIM);
                moderator.handleRequest(start);

                auto mark = forge(user_id, debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
                mark.mutable_add_claim_to_be_challenged()->set_claim_id(parentId);
                moderator.handleRequest(mark);

                auto submit = forge(user_id, debate_event::SUBMIT_CHALLENGE_CLAIM);
                auto* payload = submit.mutable_submit_challenge_claim();
                payload->set_challenge_sentence(group.claims.front()->text());
                payload->set_challenge_description(group.claims.front()->description());
                payload->set_basis(basis);
                for (size_t i = 1; i < group.claims.size(); ++i) {
                    payload->add_extra_sentences(group.claims[i]->text());
                    payload->add_extra_descriptions(group.claims[i]->description());
                }
                moderator.handleRequest(submit);
            } else {
                auto event = forge(user_id, debate_event::ADD_CHILD_CLAIM);
                auto* payload = event.mutable_add_child_claim();
                payload->set_claim(group.claims.front()->text());
                payload->set_description(group.claims.front()->description());
                payload->set_basis(basis);
                for (size_t i = 1; i < group.claims.size(); ++i) {
                    payload->add_extra_claims(group.claims[i]->text());
                    payload->add_extra_descriptions(group.claims[i]->description());
                }
                moderator.handleRequest(event);
            }
            result.argumentsCreated++;

            // Match the claims this call created back to the tree nodes that
            // asked for them, in creation order. Ids ascend in creation order,
            // and the handlers create them in the order given.
            const std::set<int> after = claimIdsIn(debateId, debateWrapper);
            std::vector<int> created;
            for (int id : after) {
                if (existing.find(id) == existing.end()) {
                    created.push_back(id);
                }
            }
            std::sort(created.begin(), created.end());
            if (created.size() != group.claims.size()) {
                result.error = "Expected " + std::to_string(group.claims.size()) +
                               " new claims but the debate gained " +
                               std::to_string(created.size()) + ".";
                return false;
            }
            existing = after;
            result.claimsCreated += static_cast<int>(created.size());

            // Recurse with each child's REAL id.
            for (size_t i = 0; i < created.size(); ++i) {
                if (!write(*group.claims[i], created[i], depth + 1)) {
                    return false;
                }
            }
            return true;
        }
    };

    Writer writer{user_id, debateId, moderator, debateWrapper, existing, result};
    if (!writer.write(tree.root(), rootId, 0)) {
        return result;
    }

    Log::info("[ExtractedDebate] Built debate " + std::to_string(debateId) + ": " +
              std::to_string(result.claimsCreated) + " claims, " +
              std::to_string(result.argumentsCreated) + " arguments, depth " +
              std::to_string(result.maxDepthReached) + ", " +
              std::to_string(result.groupsSplit) + " groups split");
    result.ok = true;
    return result;
}
