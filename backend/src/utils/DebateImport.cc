#include "DebateImport.h"

#include "AnonymousUser.h"
#include "BasisRegistry.h"
#include "debate_event.pb.h"
#include "google/protobuf/io/tokenizer.h"
#include "google/protobuf/text_format.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <string>
#include <vector>

namespace debate_import {
namespace {

using debate_test::TestAction;
using debate_test::TestScenario;

std::string trim(const std::string& s) {
    const size_t first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    const size_t last = s.find_last_not_of(" \t\r\n");
    return s.substr(first, last - first + 1);
}

std::string upper(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::toupper(c)); });
    return s;
}

bool startsWithNoCase(const std::string& s, const std::string& prefix) {
    if (s.size() < prefix.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[i])) != std::tolower(static_cast<unsigned char>(prefix[i]))) {
            return false;
        }
    }
    return true;
}

std::vector<std::string> splitLines(const std::string& text) {
    std::vector<std::string> lines;
    size_t start = 0;
    while (start <= text.size()) {
        const size_t nl = text.find('\n', start);
        std::string line = text.substr(start, nl == std::string::npos ? std::string::npos : nl - start);
        if (!line.empty() && line.back() == '\r') line.pop_back();
        lines.push_back(line);
        if (nl == std::string::npos) break;
        start = nl + 1;
    }
    // A UTF-8 byte-order mark, which some editors put in front of a file.
    if (!lines.empty() && lines[0].rfind("\xEF\xBB\xBF", 0) == 0) lines[0] = lines[0].substr(3);
    return lines;
}

std::string basesList() {
    std::string out;
    for (const std::string& name : bases::Names()) {
        if (!out.empty()) out += ", ";
        out += name;
    }
    return out;
}

// "the quote" -> the quote; also the curly quotes a word processor makes.
std::string stripQuotes(const std::string& s) {
    if (s.size() >= 2 && s.front() == '"' && s.back() == '"') return trim(s.substr(1, s.size() - 2));
    const std::string open = "\xE2\x80\x9C", close = "\xE2\x80\x9D";
    if (s.size() >= open.size() + close.size() && s.compare(0, open.size(), open) == 0 &&
        s.compare(s.size() - close.size(), close.size(), close) == 0) {
        return trim(s.substr(open.size(), s.size() - open.size() - close.size()));
    }
    return s;
}

bool isWebAddress(const std::string& url) {
    return startsWithNoCase(url, "http://") || startsWithNoCase(url, "https://");
}

void addVoice(Plan& plan, const std::string& voice) {
    const std::string v = trim(voice);
    if (v.empty()) return;
    if (std::find(plan.voices.begin(), plan.voices.end(), v) == plan.voices.end()) plan.voices.push_back(v);
}

void limitClaims(Plan& plan) {
    if (plan.claims > kMaxClaims) {
        plan.errors.push_back("The debate has " + std::to_string(plan.claims) + " claims; an import takes at most " +
                              std::to_string(kMaxClaims) + ".");
    }
}

// The protobuf parser's own words, with 1-based line and column.
class Collector : public google::protobuf::io::ErrorCollector {
public:
    std::vector<std::string> messages;
    void RecordError(int line, google::protobuf::io::ColumnNumber column, absl::string_view message) override {
        std::string where = line >= 0 ? "Line " + std::to_string(line + 1) + ", column " +
                                            std::to_string(column + 1) + ": "
                                      : std::string();
        messages.push_back(where + std::string(message));
    }
};

bool parseScenario(const std::string& text, TestScenario* scenario, std::vector<std::string>* errors) {
    Collector collector;
    google::protobuf::TextFormat::Parser parser;
    parser.RecordErrorsTo(&collector);
    const bool ok = parser.ParseFromString(text, scenario);
    if (!ok && errors != nullptr) {
        for (const std::string& m : collector.messages) errors->push_back(m);
        if (collector.messages.empty()) errors->push_back("The text is not a valid scenario file.");
    }
    return ok;
}

// The steps an import may replay. Everything here only navigates or adds to
// the debate the import itself creates; anything that deletes, edits, concedes
// or names another debate is refused rather than trusted.
const std::set<std::string>& allowedEvents() {
    static const std::set<std::string> allowed = {
        "CREATE_USER", "CREATE_DEBATE", "JOIN_DEBATE", "ENTER_DEBATE",
        "GO_TO_CLAIM", "SELECT_CLAIM", "GO_TO_PARENT", "GO_TO_OVERVIEW", "GO_TO_FULL_DEBATE_VIEW",
        "OPEN_ADD_CHILD_CLAIM", "CLOSE_ADD_CHILD_CLAIM", "ADD_CHILD_CLAIM",
        "START_CHALLENGE_CLAIM", "ADD_CLAIM_TO_BE_CHALLENGED", "REMOVE_CLAIM_TO_BE_CHALLENGED",
        "ADD_LINK_TO_BE_CHALLENGED", "OPEN_ADD_CHALLENGE", "CLOSE_ADD_CHALLENGE",
        "SUBMIT_CHALLENGE_CLAIM", "CANCEL_CHALLENGE_CLAIM",
    };
    return allowed;
}

TestAction* addStep(Plan& plan, int line, const std::string& eventType) {
    TestAction* a = plan.scenario.add_steps()->mutable_action();
    a->set_username(kOutlineActor);
    a->set_event_type(eventType);
    plan.stepLines.push_back(line);
    return a;
}

}  // namespace

bool LooksLikeScenario(const std::string& text) {
    for (const std::string& raw : splitLines(text)) {
        const std::string line = trim(raw);
        if (line.empty() || line[0] == '#') continue;
        if (line.rfind("steps", 0) == 0) {
            const std::string rest = trim(line.substr(5));
            return rest.empty() || rest[0] == '{' || rest[0] == ':';
        }
        if (line.rfind("name", 0) == 0) {
            const std::string rest = trim(line.substr(4));
            return !rest.empty() && rest[0] == ':';
        }
        return false;
    }
    return false;
}

Plan ReadOutline(const std::string& text) {
    Plan plan;
    plan.format = "outline";
    plan.actor = kOutlineActor;
    addStep(plan, 0, "CREATE_USER");

    struct Open {
        int indent;
        int id;
    };
    std::vector<Open> stack;
    int nextId = 1;
    bool haveRoot = false;

    const std::vector<std::string> lines = splitLines(text);
    for (size_t i = 0; i < lines.size(); ++i) {
        const int lineNo = static_cast<int>(i) + 1;
        const std::string& line = lines[i];
        const std::string at = "Line " + std::to_string(lineNo) + ": ";

        int indent = 0;
        size_t pos = 0;
        while (pos < line.size() && (line[pos] == ' ' || line[pos] == '\t')) {
            indent += line[pos] == '\t' ? 4 : 1;
            ++pos;
        }
        const std::string rest = trim(line.substr(pos));
        if (rest.empty() || rest[0] == '#') continue;

        const char marker = (rest[0] == '+' || rest[0] == '-') ? rest[0] : '\0';
        const std::string body = marker ? trim(rest.substr(1)) : rest;

        std::vector<std::string> fields;
        size_t start = 0;
        while (true) {
            const size_t bar = body.find('|', start);
            fields.push_back(trim(body.substr(start, bar == std::string::npos ? std::string::npos : bar - start)));
            if (bar == std::string::npos) break;
            start = bar + 1;
        }
        const std::string claim = fields[0];
        const std::string voice = fields.size() > 1 ? fields[1] : "";
        const std::string url = fields.size() > 2 ? fields[2] : "";
        const std::string quote = fields.size() > 3 ? stripQuotes(fields[3]) : "";
        const std::string basis = fields.size() > 4 ? upper(fields[4]) : "";
        const std::string note = fields.size() > 5 ? fields[5] : "";

        if (fields.size() > 6) {
            plan.errors.push_back(at + "has " + std::to_string(fields.size()) +
                                  " fields separated by |; the most is 6 (claim | voice | source | quote | "
                                  "basis | note). A claim's text cannot contain |.");
        }
        if (claim.empty()) plan.errors.push_back(at + "the claim's text is empty.");
        if (!url.empty() && !isWebAddress(url)) {
            plan.errors.push_back(at + "the source \"" + url + "\" is not a web address (http:// or https://).");
        }
        if (!quote.empty() && url.empty()) plan.warnings.push_back(at + "has a quote but no source.");
        if (anonymous_user::IsAnonymousUsername(trim(voice))) {
            plan.errors.push_back(at + "\"" + voice + "\" is a visitor's name and cannot be a voice.");
        }
        addVoice(plan, voice);

        if (!haveRoot) {
            haveRoot = true;
            if (marker) {
                plan.errors.push_back(at + "the first claim is the debate's root, so it has no + or - in front.");
            }
            if (!basis.empty()) {
                plan.warnings.push_back(at + "the root has no parent, so its basis (" + basis + ") is not used.");
            }
            if (!note.empty()) {
                plan.warnings.push_back(at + "the root's note is not stored; a debate's root claim has no description.");
            }
            plan.root = claim;
            TestAction* a = addStep(plan, lineNo, "CREATE_DEBATE");
            a->set_debate_topic(claim);
            if (!voice.empty()) a->set_voice(voice);
            if (!url.empty()) a->set_source_url(url);
            if (!quote.empty()) a->set_source_excerpt(quote);
            addStep(plan, lineNo, "JOIN_DEBATE")->set_debate_id(1);
            addStep(plan, lineNo, "ENTER_DEBATE")->set_debate_id(1);
            stack.push_back({indent, nextId++});
            ++plan.claims;
            continue;
        }

        if (!basis.empty() && !bases::IsKnown(basis)) {
            plan.errors.push_back(at + "\"" + fields[4] + "\" is not a basis; use one of " + basesList() +
                                  ", or leave it blank.");
        }
        if (!marker) {
            plan.errors.push_back(at + "start the line with + (supports the claim above it) or - (challenges it).");
        }
        while (!stack.empty() && stack.back().indent >= indent) stack.pop_back();
        if (stack.empty()) {
            plan.errors.push_back(at + "there is no claim above it with less indentation for it to " +
                                  std::string(marker == '-' ? "challenge" : "support") +
                                  "; indent it under that claim.");
            // Kept, so the lines under it still find a parent and are checked.
            stack.push_back({indent, nextId++});
            ++plan.claims;
            continue;
        }
        const int parent = stack.back().id;
        const bool challenge = marker == '-';

        addStep(plan, lineNo, "GO_TO_CLAIM")->set_claim_id(parent);
        TestAction* a = nullptr;
        if (challenge) {
            addStep(plan, lineNo, "START_CHALLENGE_CLAIM");
            addStep(plan, lineNo, "ADD_CLAIM_TO_BE_CHALLENGED")->set_claim_id(parent);
            addStep(plan, lineNo, "OPEN_ADD_CHALLENGE");
            a = addStep(plan, lineNo, "SUBMIT_CHALLENGE_CLAIM");
            if (!note.empty()) a->set_challenge_description(note);
            ++plan.challenges;
        } else {
            a = addStep(plan, lineNo, "ADD_CHILD_CLAIM");
            if (!note.empty()) a->set_claim_description(note);
            ++plan.supports;
        }
        a->set_claim_text(claim);
        if (!voice.empty()) a->set_voice(voice);
        if (!basis.empty()) a->set_basis(basis);
        if (!url.empty()) a->set_source_url(url);
        if (!quote.empty()) a->set_source_excerpt(quote);

        stack.push_back({indent, nextId++});
        ++plan.claims;
    }

    if (!haveRoot) {
        plan.errors.push_back("There is no claim in the text. The first line that is not blank or a # comment "
                              "is the debate's root claim.");
    }
    limitClaims(plan);
    return plan;
}

Plan ReadScenario(const std::string& text) {
    Plan plan;
    plan.format = "scenario";
    if (!parseScenario(text, &plan.scenario, &plan.errors)) return plan;
    if (plan.scenario.steps_size() == 0) {
        plan.errors.push_back("The scenario has no steps.");
        return plan;
    }
    plan.stepLines.assign(plan.scenario.steps_size(), 0);

    std::vector<std::string> actors;
    bool created = false;
    for (int i = 0; i < plan.scenario.steps_size(); ++i) {
        if (!plan.scenario.steps(i).has_action()) continue;  // expectations only: not replayed
        const TestAction& a = plan.scenario.steps(i).action();
        const std::string type = a.event_type();
        const std::string at = "Step " + std::to_string(i + 1) + " (" + (type.empty() ? "no event_type" : type) + "): ";
        const std::string user = trim(a.username());

        if (user.empty()) {
            plan.errors.push_back(at + "has no username.");
        } else if (type != "CREATE_USER" && std::find(actors.begin(), actors.end(), user) == actors.end()) {
            actors.push_back(user);
        }
        if (allowedEvents().count(type) == 0) {
            plan.errors.push_back(at + "an import cannot replay this kind of step; it may only create its own "
                                  "debate and add claims and challenges to it.");
            continue;
        }
        if (type == "CREATE_USER") continue;  // checked against the actor below

        if (type == "CREATE_DEBATE") {
            if (created) {
                plan.errors.push_back(at + "a second debate; an import creates exactly one.");
                continue;
            }
            created = true;
            plan.root = trim(a.debate_topic());
            if (plan.root.empty()) plan.errors.push_back(at + "the debate's root claim (debate_topic) is empty.");
            addVoice(plan, a.voice());
            ++plan.claims;
            continue;
        }
        if (!created) {
            plan.errors.push_back(at + "comes before CREATE_DEBATE; the debate must be created first.");
            continue;
        }

        auto checkClaim = [&](int id, const std::string& field) {
            if (id < 1 || id > plan.claims) {
                plan.errors.push_back(at + field + " " + std::to_string(id) + " is not one of the " +
                                      std::to_string(plan.claims) + " claim(s) this scenario has made so far.");
            }
        };
        if (type == "JOIN_DEBATE" || type == "ENTER_DEBATE") {
            if (a.debate_id() != 1) {
                plan.errors.push_back(at + "debate_id must be 1, the debate this scenario creates.");
            }
        } else if (type == "GO_TO_CLAIM" || type == "SELECT_CLAIM") {
            checkClaim(a.claim_id(), "claim_id");
        } else if (type == "ADD_CLAIM_TO_BE_CHALLENGED" || type == "REMOVE_CLAIM_TO_BE_CHALLENGED") {
            if (a.claim_id() != 0) checkClaim(a.claim_id(), "claim_id");
        } else if (type == "ADD_LINK_TO_BE_CHALLENGED") {
            checkClaim(a.link_to_claim_id(), "link_to_claim_id");
            checkClaim(a.link_from_claim_id(), "link_from_claim_id");
        } else if (type == "ADD_CHILD_CLAIM" || type == "SUBMIT_CHALLENGE_CLAIM") {
            if (trim(a.claim_text()).empty()) plan.errors.push_back(at + "claim_text is empty.");
            for (const std::string& extra : a.extra_claim_texts()) {
                if (trim(extra).empty()) plan.errors.push_back(at + "one of its extra_claim_texts is empty.");
            }
            if (!a.basis().empty() && !bases::IsKnown(a.basis())) {
                plan.errors.push_back(at + "\"" + a.basis() + "\" is not a basis; use one of " + basesList() + ".");
            }
            if (!a.source_url().empty() && !isWebAddress(a.source_url())) {
                plan.errors.push_back(at + "source_url \"" + a.source_url() + "\" is not a web address.");
            }
            if (anonymous_user::IsAnonymousUsername(trim(a.voice()))) {
                plan.errors.push_back(at + "\"" + a.voice() + "\" is a visitor's name and cannot be a voice.");
            }
            addVoice(plan, a.voice());
            const int made = 1 + a.extra_claim_texts_size();
            plan.claims += made;
            (type == "ADD_CHILD_CLAIM" ? plan.supports : plan.challenges) += made;
        }
    }

    if (!created) plan.errors.push_back("The scenario never creates a debate (no CREATE_DEBATE step).");
    if (actors.size() > 1) {
        std::string names;
        for (const std::string& n : actors) names += (names.empty() ? "" : ", ") + ("\"" + n + "\"");
        plan.errors.push_back("The scenario acts as " + std::to_string(actors.size()) + " users (" + names +
                              "). An import enters every step as you, so it must act as one; give the "
                              "others' claims a voice instead.");
    }
    if (!actors.empty()) plan.actor = actors.front();
    // CREATE_USER for anyone but the acting user would make a bare account.
    for (int i = 0; i < plan.scenario.steps_size(); ++i) {
        const TestAction& a = plan.scenario.steps(i).action();
        if (a.event_type() == "CREATE_USER" && !plan.actor.empty() && trim(a.username()) != plan.actor) {
            plan.errors.push_back("Step " + std::to_string(i + 1) + " (CREATE_USER): creates the account \"" +
                                  a.username() + "\"; an import only creates the voices its claims name.");
        }
    }
    limitClaims(plan);
    return plan;
}

Plan Read(const std::string& text) {
    if (trim(text).empty()) {
        Plan plan;
        plan.errors.push_back("Paste a scenario file or an outline first.");
        return plan;
    }
    if (text.size() > kMaxBytes) {
        Plan plan;
        plan.errors.push_back("The text is " + std::to_string((text.size() + 1023) / 1024) +
                              " KB; an import takes at most " + std::to_string(kMaxBytes / 1024) + " KB.");
        return plan;
    }
    TestScenario probe;
    if ((parseScenario(text, &probe, nullptr) && probe.steps_size() > 0) || LooksLikeScenario(text)) {
        return ReadScenario(text);
    }
    return ReadOutline(text);
}

}  // namespace debate_import
