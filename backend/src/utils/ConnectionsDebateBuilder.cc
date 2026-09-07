#include "ConnectionsDebateBuilder.h"

#include "BasisRegistry.h"
#include "DebateWrapper.h"
#include "Log.h"
#include "debate_event.pb.h"

#include <jsoncons/json.hpp>

#include <algorithm>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {

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

std::string trim(const std::string& s) {
    const size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) {
        return "";
    }
    const size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}

std::vector<std::string> lines(const std::string& text) {
    std::vector<std::string> out;
    std::istringstream in(text);
    std::string line;
    while (std::getline(in, line)) {
        out.push_back(line);
    }
    return out;
}

std::string lower(std::string s) {
    for (char& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    return s;
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

// One JSON object -> one connection, or a reason it could not be one.
bool connectionFrom(const jsoncons::json& obj, ConnectionsDebateBuilder::Connection& out,
                    std::string& why) {
    if (!obj.is_object()) {
        why = "not a JSON object";
        return false;
    }

    // "from" may be an array of numbers or, leniently, a single number.
    if (!obj.contains("from")) {
        why = "no \"from\"";
        return false;
    }
    const jsoncons::json& from = obj["from"];
    if (from.is_array()) {
        for (const auto& v : from.array_range()) {
            if (v.is_number()) {
                out.from.push_back(v.as<int>());
            }
        }
    } else if (from.is_number()) {
        out.from.push_back(from.as<int>());
    }
    if (out.from.empty()) {
        why = "\"from\" holds no claim numbers";
        return false;
    }

    auto scalarText = [&](const char* key) -> std::string {
        if (!obj.contains(key)) return "";
        const jsoncons::json& v = obj[key];
        if (v.is_string()) return trim(v.as<std::string>());
        if (v.is_number()) return std::to_string(v.as<int>());
        return "";
    };
    out.id = scalarText("id");
    out.to_link = scalarText("to_link");

    // A connection bears on a claim ("to") or on an argument ("to_link"),
    // never both and never neither.
    const bool hasTo = obj.contains("to") && obj["to"].is_number();
    if (hasTo && !out.to_link.empty()) {
        why = "has both \"to\" and \"to_link\"; a connection bears on one thing";
        return false;
    }
    if (!hasTo && out.to_link.empty()) {
        why = "no numeric \"to\" (or \"to_link\")";
        return false;
    }
    out.to = hasTo ? obj["to"].as<int>() : 0;

    const std::string role =
        obj.contains("role") && obj["role"].is_string() ? lower(obj["role"].as<std::string>()) : "";
    if (role == "supports" || role == "support" || role == "for") {
        out.opposes = false;
    } else if (role == "opposes" || role == "oppose" || role == "against" ||
               role == "challenges" || role == "challenge") {
        out.opposes = true;
    } else {
        why = "role is not supports or opposes";
        return false;
    }

    auto str = [&](const char* key) {
        return obj.contains(key) && obj[key].is_string() ? obj[key].as<std::string>()
                                                         : std::string();
    };
    out.basis = str("basis");
    out.why = str("why");
    out.joint_reason = str("joint_reason");
    return true;
}

} // namespace

std::string ConnectionsDebateBuilder::AuthorName() { return "Article"; }

std::vector<std::string> ConnectionsDebateBuilder::ParseClaims(const std::string& text) {
    // Any of the list markers a model reaches for: "1.", "1)", "1:", "[1]",
    // "#1", "-", "*", "•". Stripped rather than required, because the numbering
    // that matters is the ORDER -- the model's own numbers are not trusted, so a
    // skipped or repeated number cannot corrupt anything.
    static const std::regex marker(R"(^\s*(?:\[?\d+\]?\s*[\.\):\]-]?\s*|[-*\xE2\x80\xA2]\s+|#\s*\d+\s*[\.\):]?\s*))");
    std::vector<std::string> claims;
    for (const std::string& raw : lines(stripCodeFence(text))) {
        std::string line = trim(raw);
        if (line.empty() || line == "```") {
            continue;
        }
        line = trim(std::regex_replace(line, marker, "", std::regex_constants::format_first_only));
        // A claim the model quoted as a whole.
        if (line.size() >= 2 && line.front() == '"' && line.back() == '"') {
            line = trim(line.substr(1, line.size() - 2));
        }
        if (!line.empty()) {
            claims.push_back(line);
        }
    }
    return claims;
}

std::string ConnectionsDebateBuilder::WithRoot(const std::string& claimsText, const std::string& root) {
    if (trim(root).empty()) {
        return claimsText;
    }
    std::vector<std::string> claims = ParseClaims(claimsText);
    if (claims.empty()) {
        claims.push_back(trim(root));
    } else {
        claims[0] = trim(root);
    }
    std::ostringstream out;
    for (size_t i = 0; i < claims.size(); ++i) {
        out << (i + 1) << ". " << claims[i] << "\n";
    }
    return out.str();
}

std::vector<ConnectionsDebateBuilder::Connection> ConnectionsDebateBuilder::ParseConnections(
    const std::string& text, std::vector<std::string>& dropped) {
    std::vector<Connection> connections;
    const std::string body = trim(stripCodeFence(text));

    // The prompt asks for one object per line, but a model that has seen more
    // JSON arrays than JSON lines will hand back an array. Take either.
    if (!body.empty() && body.front() == '[') {
        try {
            const jsoncons::json arr = jsoncons::json::parse(body);
            if (arr.is_array()) {
                for (const auto& obj : arr.array_range()) {
                    Connection c;
                    std::string why;
                    if (connectionFrom(obj, c, why)) {
                        c.seq = static_cast<int>(connections.size()) + 1;  // position, from 1
                        connections.push_back(c);
                    } else {
                        dropped.push_back(why + ": " + obj.to_string().substr(0, 120));
                    }
                }
                return connections;
            }
        } catch (const std::exception&) {
            // Not a whole array after all; fall through and take it line by line.
        }
    }

    for (const std::string& raw : lines(body)) {
        std::string line = trim(raw);
        if (line.empty() || line == "[" || line == "]" || line == ",") {
            continue;
        }
        if (line.back() == ',') {
            line.pop_back();  // a trailing comma from array habits
        }
        jsoncons::json obj;
        try {
            obj = jsoncons::json::parse(line);
        } catch (const std::exception& e) {
            dropped.push_back(std::string("not JSON (") + e.what() + "): " + line.substr(0, 120));
            continue;
        }
        Connection c;
        std::string why;
        if (connectionFrom(obj, c, why)) {
            c.seq = static_cast<int>(connections.size()) + 1;  // position, from 1
            connections.push_back(c);
        } else {
            dropped.push_back(why + ": " + line.substr(0, 120));
        }
    }
    return connections;
}

ConnectionsDebateBuilder::Result ConnectionsDebateBuilder::Build(const std::string& claimsText,
                                                                 const std::string& connectionsText,
                                                                 int user_id,
                                                                 DebateModerator& moderator) {
    Result result;

    const std::vector<std::string> claims = ParseClaims(claimsText);
    const int n = static_cast<int>(claims.size());
    if (n == 0) {
        result.error = "The AI found no claims in the passage.";
        return result;
    }

    std::vector<Connection> raw = ParseConnections(connectionsText, result.dropped);

    // ---- Validate every connection against the list, before writing anything.
    //
    // A claim may be placed once: the write path creates a claim AND its link
    // to a parent in one step, so a second placement would be a second copy of
    // the claim. Root supports nothing. Numbers must be in the list.
    std::vector<Connection> usable;
    std::set<int> placed;
    std::set<int> splitSeqs;  // joint lines that became several; not a valid undercut target
    for (Connection& c : raw) {
        auto summary = [&]() {
            std::ostringstream s;
            s << "from [";
            for (size_t i = 0; i < c.from.size(); ++i) s << (i ? "," : "") << c.from[i];
            s << "] to " << c.to;
            return s.str();
        };
        bool bad = false;
        if (!c.to_link.empty()) {
            // An undercut. Resolve the attacked line: by its "id", else by its
            // position. It must exist, not be this line, and this must be a
            // challenge -- "supporting an argument" is not a move here; the
            // way to back an argument is to support the claim it is for.
            int target = 0;
            for (const Connection& other : raw) {
                if (!other.id.empty() && other.id == c.to_link) { target = other.seq; break; }
            }
            if (target == 0) {
                bool numeric = !c.to_link.empty() &&
                    std::all_of(c.to_link.begin(), c.to_link.end(), [](char ch) { return ch >= '0' && ch <= '9'; });
                const int pos = numeric ? std::stoi(c.to_link) : 0;
                if (pos >= 1 && pos <= static_cast<int>(raw.size())) target = pos;
            }
            if (target == 0) {
                result.dropped.push_back("attacks connection \"" + c.to_link +
                                         "\", which no line has as its id or position: " + summary());
                continue;
            }
            if (target == c.seq) {
                result.dropped.push_back("attacks itself: " + summary());
                continue;
            }
            if (!c.opposes) {
                result.dropped.push_back("\"supports\" an argument; to back an argument, support the "
                                         "claim it is for: " + summary());
                continue;
            }
            c.target_seq = target;
        } else if (c.to < 1 || c.to > n) {
            result.dropped.push_back("targets claim " + std::to_string(c.to) +
                                     ", which is not in the list: " + summary());
            continue;
        }
        for (int f : c.from) {
            if (f < 1 || f > n) {
                result.dropped.push_back("names claim " + std::to_string(f) +
                                         ", which is not in the list: " + summary());
                bad = true;
                break;
            }
            if (f == 1) {
                result.dropped.push_back("puts the root in \"from\"; the root supports nothing: " +
                                         summary());
                bad = true;
                break;
            }
            if (c.to != 0 && f == c.to) {
                result.dropped.push_back("connects claim " + std::to_string(f) + " to itself");
                bad = true;
                break;
            }
            if (placed.count(f)) {
                result.dropped.push_back("claim " + std::to_string(f) +
                                         " was already placed by an earlier connection: " +
                                         summary());
                bad = true;
                break;
            }
        }
        if (bad) {
            continue;
        }

        // Several "from" without a stated reason are split. The model groups by
        // default and groups wrongly; with nothing stated the conservative
        // reading is independent, the opposite of the way it errs.
        if (c.from.size() > 1 && trim(c.joint_reason).empty()) {
            result.groupsSplit++;
            splitSeqs.insert(c.seq);
            for (int f : c.from) {
                Connection single = c;
                single.from = {f};
                usable.push_back(single);
                placed.insert(f);
            }
            continue;
        }
        for (int f : c.from) {
            placed.insert(f);
        }
        usable.push_back(c);
    }

    // An undercut aimed at a joint line that was split has no single argument
    // to attack any more. Dropped and said so, rather than guessing a half.
    {
        std::vector<Connection> kept;
        for (const Connection& c : usable) {
            if (c.target_seq > 0 && splitSeqs.count(c.target_seq)) {
                result.dropped.push_back("attacks connection " + std::to_string(c.target_seq) +
                                         ", a joint line that was split; there is no one argument to attack");
                for (int f : c.from) placed.erase(f);
                continue;
            }
            kept.push_back(c);
        }
        usable.swap(kept);
    }

    // ---- Hubs the model forgot.
    //
    // A claim that other connections target but that no connection places is
    // not a stray leaf; it is a node with a subtree, and leaving it out drops
    // the subtree. Attach it under the root with no basis. That IS a guess, and
    // it is reported as one -- but a loose branch the user can move beats a
    // silently missing third of the argument.
    {
        std::set<int> targeted;
        for (const Connection& c : usable) {
            if (c.to > 0) targeted.insert(c.to);
        }
        for (int k = 2; k <= n; ++k) {
            if (!placed.count(k) && targeted.count(k)) {
                Connection hub;
                hub.from = {k};
                hub.to = 1;
                hub.opposes = false;
                usable.push_back(hub);
                placed.insert(k);
                result.attachedToRoot++;
            }
        }
    }

    if (usable.empty() && n > 1) {
        result.error = "The AI listed " + std::to_string(n) +
                       " claims but none of its connections between them could be used.";
        return result;
    }

    // ---- Create the debate. Only now; everything above was checkable first.
    //
    // Created AS the persona, not as the requester -- see the header. The
    // requester is joined below so the debate is in their list.
    DebateWrapper& debateWrapper = moderator.getDebateWrapper();
    const int author = moderator.createUserIfNotExist(AuthorName());
    if (author <= 0) {
        result.error = "Could not create the '" + AuthorName() + "' account that authors generated debates.";
        return result;
    }
    const std::vector<int> debatesBefore = debateWrapper.getUserDebateIds(author);
    {
        auto event = forge(author, debate_event::CREATE_DEBATE);
        event.mutable_create_debate()->set_debate_topic(claims[0]);
        moderator.handleRequest(event);
    }
    int debateId = 0;
    for (int id : debateWrapper.getUserDebateIds(author)) {
        if (std::find(debatesBefore.begin(), debatesBefore.end(), id) == debatesBefore.end()) {
            debateId = id;
        }
    }
    if (debateId <= 0) {
        result.error = "The debate could not be created.";
        return result;
    }
    result.debateId = debateId;

    // AddDebate does not put the author inside the debate, and the write
    // handlers act on where the user IS.
    {
        auto event = forge(author, debate_event::ENTER_DEBATE);
        event.mutable_enter_debate()->set_debate_id(debateId);
        moderator.handleRequest(event);
    }
    // The requester becomes a member -- so the debate appears in their list --
    // but authors nothing. Done before the claims are written, so even a write
    // that fails partway leaves them holding what was built.
    {
        auto join = forge(user_id, debate_event::JOIN_DEBATE);
        join.mutable_join_debate()->set_debate_id(debateId);
        moderator.handleRequest(join);
    }

    std::set<int> existing = claimIdsIn(debateId, debateWrapper);
    if (existing.size() != 1) {
        result.error = "The new debate did not come back with exactly one root claim.";
        return result;
    }

    std::map<int, int> realId{{1, *existing.begin()}};   // list number -> claim id
    std::map<int, int> depth{{1, 0}};
    result.claimsCreated = 1;

    // Per written line: the relation it created, and the claim it stands on.
    // An undercut needs both from the line it attacks -- the relation to aim at,
    // and the claim the modal has to be standing on to find that relation.
    std::map<int, int> relationOfSeq;   // line -> relation id
    std::map<int, int> standingOfSeq;   // line -> real id of the claim the argument is for
    std::map<int, int> depthOfSeq;      // line -> depth of the claims it created

    // ---- Write connections in dependency order.
    //
    // A connection can be written once its target exists. Passes repeat until
    // one makes no progress; whatever is left then targets something that was
    // never created -- a cycle, or a target that itself sat in a dropped line.
    std::vector<bool> written(usable.size(), false);
    bool progress = true;
    while (progress) {
        progress = false;
        for (size_t i = 0; i < usable.size(); ++i) {
            if (written[i]) {
                continue;
            }
            Connection c = usable[i];  // a copy: a broken cycle can shrink `from`
            int parentId = 0;           // the claim to stand on when writing
            int attackedRelation = 0;   // for an undercut: the relation to aim at
            if (c.target_seq > 0) {
                auto relIt = relationOfSeq.find(c.target_seq);
                if (relIt == relationOfSeq.end()) {
                    continue;  // the attacked line is not written yet
                }
                attackedRelation = relIt->second;
                parentId = standingOfSeq[c.target_seq];
            } else {
                auto targetIt = realId.find(c.to);
                if (targetIt == realId.end()) {
                    continue;
                }
                parentId = targetIt->second;
            }

            // A cycle broken below has already created one of this connection's
            // sources. It must not be created twice; the edge that would close
            // the circle is the one that goes.
            {
                std::vector<int> fresh;
                for (int f : c.from) {
                    if (!realId.count(f)) {
                        fresh.push_back(f);
                    }
                }
                if (fresh.empty()) {
                    written[i] = true;
                    progress = true;
                    result.dropped.push_back("claim " + std::to_string(c.from[0]) + " -> " +
                                             std::to_string(c.to) + " would close a cycle; " +
                                             std::to_string(c.from[0]) +
                                             " was attached under the root instead");
                    continue;
                }
                c.from = fresh;
            }

            const std::string basis = bases::Sanitize(c.basis);
            if (basis.empty() && !trim(c.basis).empty()) {
                result.unknownBases++;
            }

            {
                auto event = forge(author, debate_event::GO_TO_CLAIM);
                event.mutable_go_to_claim()->set_claim_id(parentId);
                moderator.handleRequest(event);
            }
            if (c.opposes) {
                auto start = forge(author, debate_event::START_CHALLENGE_CLAIM);
                moderator.handleRequest(start);
                if (attackedRelation > 0) {
                    // Aim at the argument, exactly as the modal does when a
                    // pill is picked.
                    auto mark = forge(author, debate_event::ADD_LINK_TO_BE_CHALLENGED);
                    mark.mutable_add_link_to_be_challenged()->set_link_id(attackedRelation);
                    moderator.handleRequest(mark);
                } else {
                    auto mark = forge(author, debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
                    mark.mutable_add_claim_to_be_challenged()->set_claim_id(parentId);
                    moderator.handleRequest(mark);
                }
                auto submit = forge(author, debate_event::SUBMIT_CHALLENGE_CLAIM);
                auto* payload = submit.mutable_submit_challenge_claim();
                payload->set_challenge_sentence(claims[c.from[0] - 1]);
                payload->set_challenge_description(c.why);
                payload->set_basis(basis);
                for (size_t k = 1; k < c.from.size(); ++k) {
                    payload->add_extra_sentences(claims[c.from[k] - 1]);
                    payload->add_extra_descriptions("");
                }
                moderator.handleRequest(submit);
            } else {
                auto event = forge(author, debate_event::ADD_CHILD_CLAIM);
                auto* payload = event.mutable_add_child_claim();
                payload->set_claim(claims[c.from[0] - 1]);
                payload->set_description(c.why);
                payload->set_basis(basis);
                for (size_t k = 1; k < c.from.size(); ++k) {
                    payload->add_extra_claims(claims[c.from[k] - 1]);
                    payload->add_extra_descriptions("");
                }
                moderator.handleRequest(event);
            }

            const std::set<int> after = claimIdsIn(debateId, debateWrapper);
            std::vector<int> created;
            for (int id : after) {
                if (!existing.count(id)) {
                    created.push_back(id);
                }
            }
            std::sort(created.begin(), created.end());
            if (created.size() != c.from.size()) {
                result.error = "Expected " + std::to_string(c.from.size()) +
                               " new claims but the debate gained " +
                               std::to_string(created.size()) + ".";
                return result;
            }
            existing = after;
            // Depth: one below the claim this bears on, or -- for an undercut --
            // one below the claims of the argument it attacks, so an undercut
            // of an undercut is deeper than the undercut, as it is drawn.
            const int newDepth = c.target_seq > 0 ? depthOfSeq[c.target_seq] + 1 : depth[c.to] + 1;
            for (size_t k = 0; k < created.size(); ++k) {
                realId[c.from[k]] = created[k];
                depth[c.from[k]] = newDepth;
                result.maxDepthReached = std::max(result.maxDepthReached, newDepth);
            }
            if (c.seq > 0) depthOfSeq[c.seq] = newDepth;
            // Remember what this line made, for any line that attacks it.
            if (c.seq > 0) {
                standingOfSeq[c.seq] = parentId;
                debate::Relationship made = c.opposes
                    ? debateWrapper.findOutgoingChallengeLink(created[0])
                    : debate::Relationship();
                if (!c.opposes) {
                    for (int linkId : debateWrapper.findLinksUnder(parentId)) {
                        const debate::Relationship rel = debateWrapper.getLinkById(linkId);
                        if (rel.link().connect_to() == created[0]) { made = rel; break; }
                    }
                }
                if (made.id() > 0) relationOfSeq[c.seq] = made.id();
            }
            result.claimsCreated += static_cast<int>(created.size());
            result.argumentsCreated++;
            if (attackedRelation > 0) result.undercuts++;
            written[i] = true;
            progress = true;
        }

        // ---- Cycles.
        //
        // A pass that wrote nothing while connections remain means every one
        // of them waits on a target that will never be created -- they wait on
        // each other. Break the circle where it costs least: the claim the
        // most connections point at goes under the root (a guess, reported as
        // one), and the next pass unwinds the rest. Without this a three-claim
        // circle took seven of thirty claims with it in a live run.
        if (!progress) {
            std::map<int, int> waiting;  // uncreated target -> connections needing it
            for (size_t i = 0; i < usable.size(); ++i) {
                const int t = usable[i].to;
                if (!written[i] && !realId.count(t) && t >= 2 && t <= n) {
                    waiting[t]++;
                }
            }
            int pick = 0, most = 0;
            for (const auto& w : waiting) {
                if (w.second > most) {
                    most = w.second;
                    pick = w.first;
                }
            }
            if (pick != 0) {
                Connection hub;
                hub.from = {pick};
                hub.to = 1;
                hub.opposes = false;
                usable.push_back(hub);
                written.push_back(false);
                result.attachedToRoot++;
                result.cyclesBroken++;
                progress = true;
            }
        }
    }

    for (size_t i = 0; i < usable.size(); ++i) {
        if (!written[i]) {
            if (usable[i].target_seq > 0) {
                result.dropped.push_back("attacks connection " + std::to_string(usable[i].target_seq) +
                                         ", which was never written");
            } else {
                result.dropped.push_back("targets claim " + std::to_string(usable[i].to) +
                                         ", which was never created (a cycle, or its own "
                                         "connection was dropped)");
            }
        }
    }
    for (int k = 2; k <= n; ++k) {
        if (!realId.count(k)) {
            result.orphans.push_back(k);
        }
    }

    Log::info("[Connections] Built debate " + std::to_string(debateId) + ": " +
              std::to_string(result.claimsCreated) + "/" + std::to_string(n) + " claims, " +
              std::to_string(result.argumentsCreated) + " arguments, depth " +
              std::to_string(result.maxDepthReached) + ", " +
              std::to_string(result.dropped.size()) + " dropped, " +
              std::to_string(result.orphans.size()) + " orphans");
    result.ok = true;
    return result;
}
