#include "HomePageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../utils/DemoMode.h"
#include "../../../../utils/AnonymousUser.h"
#include "../../../../utils/TutorialLibrary.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <utility>
#include <vector>

// The home page has three tabs, picked in the header (LayoutGenerator) and
// remembered on the reader's engagement.home_view:
//
//   Home          one featured case, the latest moves across every debate,
//                 and a few other cases.
//   Explore       every debate as a one-line row, with a search, filters and
//                 a sort.
//   Your debates  (signed in only) the challenges waiting on the reader, and
//                 the debates they are a member of.
//
// Everything shown comes from the per-debate cards the moderator caches
// (HomePageResponseGenerator): nothing here reads a debate.

namespace {

using Topic = rendering_info::HomeDebateTopicRenderInfo;
using Rules = std::vector<std::pair<std::string, std::string>>;

void css(ui::Component* component, const Rules& rules) {
    for (const auto& rule : rules) {
        (*component->mutable_css())[rule.first] = rule.second;
    }
}

ui::Component text(const std::string& id, const std::string& words, const Rules& rules) {
    ui::Component component = ComponentGenerator::createText(id, words, "", "", "", "");
    css(&component, rules);
    return component;
}

ui::Component box(const std::string& id, const Rules& rules) {
    ui::Component component = ComponentGenerator::createContainer(id, "w-full", "", "", "", "", "", "");
    css(&component, rules);
    return component;
}

const Rules kColumn = {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0"}};
const Rules kCaption = {
    {"color", "#9ca3af"}, {"font-size", "0.8rem"}, {"font-weight", "600"}, {"margin", "0"},
};
const Rules kMuted = {
    {"color", "#9ca3af"}, {"font-size", "0.85rem"}, {"font-weight", "400"}, {"line-height", "1.5"}, {"margin", "0"},
};
const char* kOpenColor = "#ef4444";
const char* kSettledColor = "#22c55e";
const char* kMiddot = "  \xC2\xB7  ";

// Cut to at most maxBytes without splitting a UTF-8 character (a string
// field must stay valid UTF-8), at a word break when there is one nearby.
std::string truncate(const std::string& s, size_t maxBytes) {
    if (s.size() <= maxBytes) return s;
    size_t cut = maxBytes;
    while (cut > 0 && (static_cast<unsigned char>(s[cut]) & 0xC0) == 0x80) --cut;
    const size_t space = s.rfind(' ', cut);
    if (space != std::string::npos && space + 24 > cut) cut = space;
    return s.substr(0, cut) + "...";
}

std::string count(int n, const std::string& one, const std::string& many) {
    return std::to_string(n) + " " + (n == 1 ? one : many);
}

// "just now", "5m ago", "3h ago", "2d ago", "4mo ago", "1y ago".
std::string ago(long long at, long long now) {
    if (at <= 0) return "";
    const long long s = std::max<long long>(0, now - at);
    if (s < 60) return "just now";
    if (s < 3600) return std::to_string(s / 60) + "m ago";
    if (s < 86400) return std::to_string(s / 3600) + "h ago";
    if (s < 86400LL * 30) return std::to_string(s / 86400) + "d ago";
    if (s < 86400LL * 365) return std::to_string(s / (86400LL * 30)) + "mo ago";
    return std::to_string(s / (86400LL * 365)) + "y ago";
}

std::string titleOf(const Topic& t) {
    return t.root_text().empty() ? t.topic() : t.root_text();
}

std::string idOf(const Topic& t) {
    return std::to_string(t.debate_root_claim_id());
}

bool isOpen(const Topic& t) {
    return t.open_challenge_count() > 0 || t.owing_size() > 0;
}

// Whose move it is: the party owing the most replies.
std::string turnOf(const Topic& t) {
    if (t.owing_size() == 0) return "Settled";
    return t.owing(0).who() + "'s move";
}

// A small round status dot: red while a challenge is open, green otherwise.
ui::Component dot(const std::string& id, bool open) {
    ui::Component d = ComponentGenerator::createContainer(id, "", "", "", "", "", "", "");
    css(&d, {{"width", "0.55rem"}, {"height", "0.55rem"}, {"min-width", "0.55rem"},
             {"border-radius", "9999px"}, {"background", open ? kOpenColor : kSettledColor},
             {"flex", "0 0 auto"}, {"display", "block"}, {"gap", "0"}});
    return d;
}

// A quiet text-style button: a link that is a server press.
ui::Component linkButton(const std::string& id, const std::string& label) {
    ui::Component b = ComponentGenerator::createButton(id, label, "", "", "", "", "", "", "text-sm");
    css(&b, {{"background", "transparent"}, {"border", "none"}, {"color", "#60a5fa"},
             {"cursor", "pointer"}, {"align-self", "flex-start"}, {"text-decoration", "underline"},
             {"text-underline-offset", "3px"}});
    return b;
}

// A pill: a filter or a sort choice; the lit one is the current choice.
ui::Component pill(const std::string& id, const std::string& label, bool active) {
    ui::Component b = ComponentGenerator::createButton(id, label, "", "", "", "", "", "", "text-xs rounded-full");
    css(&b, {{"background", active ? "#1f2937" : "transparent"},
             {"border", active ? "1px solid #38bdf8" : "1px solid #374151"},
             {"color", active ? "#f9fafb" : "#9ca3af"}, {"cursor", "pointer"}});
    return b;
}

// One debate as one line: dot, title (and, when asked, a meta line and the
// claims that matched a search), whose move it is on the right. The whole
// row is one press (data-click), opening the debate on its Brief tab.
// Delete, for the creator only, is a quiet control the row's press ignores.
ui::Component caseRow(const std::string& context, const Topic& t, long long now, bool withMeta,
                      bool canDelete) {
    const std::string id = idOf(t);
    ui::Component row = box("openBrief_" + context + "_" + id, {
        {"display", "flex"}, {"flex-direction", "row"}, {"align-items", "center"}, {"gap", "0.75rem"},
        {"padding", "0.65rem 0.25rem"}, {"border-bottom", "1px solid #374151"},
    });
    ComponentGenerator::addAttribute(&row, "data-click", "true");
    ComponentGenerator::addChild(&row, dot("caseDot_" + context + "_" + id, isOpen(t)));

    ui::Component middle = ComponentGenerator::createContainer("caseText_" + context + "_" + id, "", "", "", "", "", "", "");
    css(&middle, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.15rem"},
                  {"flex", "1 1 auto"}, {"min-width", "0"}});
    ComponentGenerator::addChild(&middle, text("caseTitle_" + context + "_" + id, truncate(titleOf(t), 140),
        {{"color", "#f3f4f6"}, {"font-size", "0.92rem"}, {"font-weight", "600"}, {"line-height", "1.35"},
         {"margin", "0"}, {"white-space", "nowrap"}, {"overflow", "hidden"}, {"text-overflow", "ellipsis"}}));
    if (withMeta) {
        std::string meta = count(t.claim_count(), "claim", "claims") + kMiddot +
                           count(t.people_count(), "person", "people");
        const std::string moved = ago(t.last_moved_at(), now);
        if (!moved.empty()) meta += std::string(kMiddot) + "moved " + moved;
        ComponentGenerator::addChild(&middle, text("caseMeta_" + context + "_" + id, meta,
            {{"color", "#6b7280"}, {"font-size", "0.75rem"}, {"font-weight", "400"}, {"margin", "0"}}));
    }
    int matchIndex = 0;
    for (const std::string& match : t.matched_claims()) {
        ComponentGenerator::addChild(&middle, text(
            "caseMatch_" + context + "_" + id + "_" + std::to_string(matchIndex++),
            "\"" + truncate(match, 150) + "\"",
            {{"color", "#9ca3af"}, {"font-size", "0.75rem"}, {"font-weight", "400"}, {"margin", "0"},
             {"font-style", "italic"}, {"white-space", "nowrap"}, {"overflow", "hidden"},
             {"text-overflow", "ellipsis"}}));
    }
    ComponentGenerator::addChild(&row, middle);

    ComponentGenerator::addChild(&row, text("caseTurn_" + context + "_" + id, turnOf(t),
        {{"color", isOpen(t) ? "#fca5a5" : "#86efac"}, {"font-size", "0.8rem"}, {"font-weight", "500"},
         {"margin", "0"}, {"white-space", "nowrap"}, {"flex", "0 0 auto"}}));
    if (canDelete) {
        ui::Component del = ComponentGenerator::createButton(
            "deleteDebateButton_" + id, "Delete", "", "", "", "", "", "", "text-xs");
        css(&del, {{"background", "transparent"}, {"border", "none"}, {"color", "#6b7280"},
                   {"flex", "0 0 auto"}});
        ComponentGenerator::addChild(&row, del);
    }
    return row;
}

ui::Component section(const std::string& id, const std::string& caption) {
    ui::Component s = box(id, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.35rem"}});
    ComponentGenerator::addChild(&s, text(id + "Caption", caption, kCaption));
    return s;
}

// Last moved first; the bigger debate first among equals.
void sortByMoved(std::vector<const Topic*>& topics) {
    std::stable_sort(topics.begin(), topics.end(), [](const Topic* a, const Topic* b) {
        if (a->last_moved_at() != b->last_moved_at()) return a->last_moved_at() > b->last_moved_at();
        return a->claim_count() > b->claim_count();
    });
}

// The featured case: the debate named by DEBATE_FEATURED_ID when it is set
// and exists, else the debate with the most claims (the oldest among equals).
const Topic* featuredOf(const std::vector<const Topic*>& topics) {
    if (const char* env = std::getenv("DEBATE_FEATURED_ID")) {
        const int wanted = std::atoi(env);
        for (const Topic* t : topics) {
            if (wanted > 0 && t->debate_root_claim_id() == wanted) return t;
        }
    }
    const Topic* best = nullptr;
    for (const Topic* t : topics) {
        if (best == nullptr || t->claim_count() > best->claim_count() ||
            (t->claim_count() == best->claim_count() &&
             t->debate_root_claim_id() < best->debate_root_claim_id())) {
            best = t;
        }
    }
    return best;
}

// "Paul owes a reply" / "Paul and Ann owe a reply": the parties with the
// most open challenges, at most two.
std::string owingLine(const Topic& t) {
    std::vector<std::string> names;
    for (const auto& party : t.owing()) {
        if (party.count() < t.owing(0).count() || names.size() == 2) break;
        names.push_back(party.who());
    }
    if (names.empty()) return "Settled. Nobody owes a reply.";
    if (names.size() == 1) return names[0] + " owes a reply";
    return names[0] + " and " + names[1] + " owe a reply";
}

ui::Component featuredCase(const Topic& t) {
    const std::string id = idOf(t);
    ui::Component hero = box("featuredCase", {
        {"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.6rem"},
        {"background", "#111827"}, {"border", "1px solid #374151"},
        {"border-radius", "0.75rem"}, {"padding", "1.5rem 1.5rem 1.25rem"},
    });
    ComponentGenerator::addChild(&hero, text("featuredCaption", "Featured case", kCaption));
    ComponentGenerator::addChild(&hero, text("featuredClaim", truncate(titleOf(t), 300),
        {{"color", "#f9fafb"}, {"font-size", "1.5rem"}, {"font-weight", "700"}, {"line-height", "1.3"},
         {"margin", "0"}}));
    if (!t.root_holder().empty()) {
        ComponentGenerator::addChild(&hero, text("featuredHolder", "Claimed by " + t.root_holder(), kMuted));
    }

    // Where it stands, in a sentence: the Brief's computed verdict.
    const std::string verdict = t.unresolved()
        ? "Still unresolved. It hangs on \"" + truncate(t.hinge_text(), 160) + "\""
        : "No open challenges. Every challenge so far has an answer.";
    ComponentGenerator::addChild(&hero, text("featuredVerdict", verdict,
        {{"color", "#d1d5db"}, {"font-size", "0.95rem"}, {"font-weight", "400"}, {"line-height", "1.5"},
         {"margin", "0"}}));

    ui::Component status = box("featuredStatus", {
        {"display", "flex"}, {"flex-direction", "row"}, {"align-items", "center"}, {"gap", "0.5rem"},
    });
    ComponentGenerator::addChild(&status, dot("featuredDot", isOpen(t)));
    ComponentGenerator::addChild(&status, text("featuredOwing", owingLine(t),
        {{"color", isOpen(t) ? "#fca5a5" : "#86efac"}, {"font-size", "0.85rem"}, {"font-weight", "500"},
         {"margin", "0"}}));
    ComponentGenerator::addChild(&hero, status);

    ui::Component read = ComponentGenerator::createButton(
        "openBrief_featured_" + id, "Read the brief", "", "bg-blue-600", "hover:bg-blue-700", "text-white",
        "px-4 py-2", "rounded-lg", "text-sm");
    css(&read, {{"align-self", "flex-start"}, {"margin-top", "0.35rem"}});
    ComponentGenerator::addChild(&hero, read);
    return hero;
}

// The newest moves across every debate, newest first.
ui::Component latestMoves(const std::vector<const Topic*>& topics, long long now) {
    struct Line { const Topic* topic; const moderator_to_vr::RecentMove* move; };
    std::vector<Line> lines;
    for (const Topic* t : topics) {
        for (const auto& m : t->recent_moves()) lines.push_back({t, &m});
    }
    std::stable_sort(lines.begin(), lines.end(), [](const Line& a, const Line& b) {
        if (a.move->at() != b.move->at()) return a.move->at() > b.move->at();
        return a.move->seq() > b.move->seq();
    });

    ui::Component s = section("latestMoves", "Latest moves");
    int n = 0;
    for (const Line& line : lines) {
        if (n == 4) break;
        const std::string id = idOf(*line.topic);
        ui::Component row = box("openBrief_move" + std::to_string(n) + "_" + id, {
            {"display", "flex"}, {"flex-direction", "row"}, {"align-items", "baseline"}, {"gap", "0.4rem"},
            {"padding", "0.3rem 0.25rem"},
        });
        ComponentGenerator::addAttribute(&row, "data-click", "true");
        ComponentGenerator::addChild(&row, text("latestMoveText_" + std::to_string(n),
            line.move->who() + " " + line.move->verb() + " in " + truncate(titleOf(*line.topic), 70),
            {{"color", "#d1d5db"}, {"font-size", "0.85rem"}, {"font-weight", "400"}, {"margin", "0"},
             {"white-space", "nowrap"}, {"overflow", "hidden"}, {"text-overflow", "ellipsis"},
             {"min-width", "0"}, {"flex", "0 1 auto"}}));
        const std::string when = ago(line.move->at(), now);
        if (!when.empty()) {
            ComponentGenerator::addChild(&row, text("latestMoveWhen_" + std::to_string(n),
                "\xC2\xB7 " + when,
                {{"color", "#6b7280"}, {"font-size", "0.8rem"}, {"font-weight", "400"}, {"margin", "0"},
                 {"white-space", "nowrap"}, {"flex", "0 0 auto"}}));
        }
        ComponentGenerator::addChild(&s, row);
        ++n;
    }
    if (n == 0) {
        ComponentGenerator::addChild(&s, text("latestMovesEmpty", "No moves yet.", kMuted));
    }
    return s;
}

void addHomeTab(ui::Component* page, const std::vector<const Topic*>& topics,
                const rendering_info::HomePageRenderingInfo& info, long long now) {
    const Topic* featured = featuredOf(topics);
    if (featured == nullptr) {
        ComponentGenerator::addChild(page, text("debateShelfEmpty", "No debates yet.", kMuted));
        return;
    }
    ComponentGenerator::addChild(page, featuredCase(*featured));

    // A quiet way into the walkthrough. A visitor's press opens the sign-in
    // modal and is replayed once they are signed in (START_TUTORIAL is a
    // pending action).
    const std::string walkthrough = tutorials::DefaultName();
    if (info.can_create_or_join_debates() && !walkthrough.empty()) {
        ui::Component link = linkButton("startTutorialButton_" + walkthrough, "New here? Watch one get built");
        css(&link, {{"margin-top", "-0.75rem"}});
        ComponentGenerator::addChild(page, link);
    }

    // Hidden for now: with every real debate imported in one sitting, the
    // strip only ever shows the import. Flip this on once people make
    // moves of their own.
    constexpr bool kShowLatestMoves = false;
    if (kShowLatestMoves) {
        ComponentGenerator::addChild(page, latestMoves(topics, now));
    }

    // Real cases only: a debate with fewer than 3 claims (mostly tests) is
    // left to Explore, unless there are no others.
    std::vector<const Topic*> others;
    for (const Topic* t : topics) {
        if (t != featured && t->claim_count() >= 3) others.push_back(t);
    }
    if (others.empty()) {
        for (const Topic* t : topics) {
            if (t != featured) others.push_back(t);
        }
    }
    sortByMoved(others);
    ui::Component s = section("otherCases", "Other cases");
    for (size_t i = 0; i < others.size() && i < 5; ++i) {
        ComponentGenerator::addChild(&s, caseRow("other", *others[i], now, false, false));
    }
    ui::Component seeAll = linkButton("homeTab_explore_all",
        "See all " + std::to_string(topics.size()) + " in Explore");
    css(&seeAll, {{"margin-top", "0.4rem"}});
    ComponentGenerator::addChild(&s, seeAll);
    ComponentGenerator::addChild(page, s);
}

void addExploreTab(ui::Component* page, const std::vector<const Topic*>& topics,
                   const rendering_info::HomePageRenderingInfo& info, long long now,
                   const std::function<bool(const Topic&)>& canDelete) {
    const user_engagement::HomeView& view = info.home_view();
    const std::string& filter = view.explore_filter();
    const bool byClaims = view.explore_sort() == "claims";

    // Search: the box, its button (Enter in the box sends it too).
    ui::Component search = box("exploreSearch", {
        {"display", "flex"}, {"flex-direction", "row"}, {"gap", "0.5rem"}, {"align-items", "center"},
    });
    ui::Component input = ComponentGenerator::createInput(
        "exploreSearchInput", "Search claims and titles", "query", "bg-gray-800", "text-white",
        "border-gray-600", "p-2", "rounded", "flex-1", view.explore_query());
    css(&input, {{"width", "100%"}});
    ui::Component inputWrap = ComponentGenerator::createContainer("exploreSearchInputWrap", "", "", "", "", "", "", "");
    css(&inputWrap, {{"flex", "1 1 auto"}, {"min-width", "0"}, {"display", "block"}});
    ComponentGenerator::addChild(&inputWrap, input);
    ComponentGenerator::addChild(&search, inputWrap);
    ComponentGenerator::addChild(&search, ComponentGenerator::createButton(
        "exploreSearchButton", "Search", "", "bg-blue-600", "hover:bg-blue-700", "text-white",
        "px-4 py-2", "rounded", "text-sm"));
    if (!view.explore_query().empty()) {
        ui::Component clear = ComponentGenerator::createButton(
            "exploreClearSearch", "Clear", "", "", "", "", "", "", "text-sm");
        css(&clear, {{"background", "transparent"}, {"border", "1px solid #374151"}, {"color", "#9ca3af"}});
        ComponentGenerator::addChild(&search, clear);
    }
    ComponentGenerator::addChild(page, search);

    // Filters on the left, sort on the right.
    ui::Component controls = box("exploreControls", {
        {"display", "flex"}, {"flex-direction", "row"}, {"flex-wrap", "wrap"}, {"gap", "0.4rem"},
        {"align-items", "center"}, {"justify-content", "space-between"},
    });
    ui::Component filters = ComponentGenerator::createContainer("exploreFilters", "", "", "", "", "", "", "");
    css(&filters, {{"display", "flex"}, {"flex-direction", "row"}, {"flex-wrap", "wrap"}, {"gap", "0.4rem"}});
    ComponentGenerator::addChild(&filters, pill("exploreFilter_all", "All", filter.empty()));
    ComponentGenerator::addChild(&filters, pill("exploreFilter_open", "Open", filter == "open"));
    ComponentGenerator::addChild(&filters, pill("exploreFilter_settled", "Settled", filter == "settled"));
    ComponentGenerator::addChild(&filters, pill("exploreFilter_waiting", "Waiting on someone", filter == "waiting"));
    ComponentGenerator::addChild(&controls, filters);
    ui::Component sorts = ComponentGenerator::createContainer("exploreSorts", "", "", "", "", "", "", "");
    css(&sorts, {{"display", "flex"}, {"flex-direction", "row"}, {"gap", "0.4rem"}, {"align-items", "center"}});
    ComponentGenerator::addChild(&sorts, text("exploreSortLabel", "Sort by",
        {{"color", "#6b7280"}, {"font-size", "0.78rem"}, {"font-weight", "400"}, {"margin", "0"}}));
    ComponentGenerator::addChild(&sorts, pill("exploreSort_moved", "Latest move", !byClaims));
    ComponentGenerator::addChild(&sorts, pill("exploreSort_claims", "Most claims", byClaims));
    ComponentGenerator::addChild(&controls, sorts);
    ComponentGenerator::addChild(page, controls);

    std::vector<const Topic*> shown;
    for (const Topic* t : topics) {
        if (!view.explore_query().empty() && !t->search_match()) continue;
        if (filter == "open" && !isOpen(*t)) continue;
        if (filter == "settled" && isOpen(*t)) continue;
        if (filter == "waiting" && t->owing_size() == 0) continue;
        shown.push_back(t);
    }
    sortByMoved(shown);
    if (byClaims) {
        std::stable_sort(shown.begin(), shown.end(), [](const Topic* a, const Topic* b) {
            return a->claim_count() > b->claim_count();
        });
    }

    ui::Component list = box("exploreList", kColumn);
    for (const Topic* t : shown) {
        ComponentGenerator::addChild(&list, caseRow("explore", *t, now, true, canDelete(*t)));
    }
    if (shown.empty()) {
        ComponentGenerator::addChild(&list, text("exploreEmpty",
            view.explore_query().empty() ? "No cases match this filter."
                                         : "No cases match \"" + truncate(view.explore_query(), 60) + "\".",
            kMuted));
    }
    ComponentGenerator::addChild(page, list);
    ComponentGenerator::addChild(page, text("exploreCount", count(static_cast<int>(shown.size()), "case", "cases"),
        {{"color", "#6b7280"}, {"font-size", "0.8rem"}, {"font-weight", "400"}, {"margin", "0"}}));
}

void addMineTab(ui::Component* page, const std::vector<const Topic*>& topics, long long now,
                const std::function<bool(const Topic&)>& canDelete) {
    std::vector<const Topic*> mine;
    for (const Topic* t : topics) {
        if (t->viewer_is_member() || t->owed_to_viewer_size() > 0) mine.push_back(t);
    }
    sortByMoved(mine);

    // Every challenge the reader owes a reply to, across debates.
    ui::Component waiting = section("waitingOnYou", "Waiting on you");
    int n = 0;
    for (const Topic* t : mine) {
        for (const auto& owed : t->owed_to_viewer()) {
            const std::string key = idOf(*t) + "_" + std::to_string(owed.claim_id());
            ui::Component row = box("waitingRow_" + key, {
                {"display", "flex"}, {"flex-direction", "row"}, {"align-items", "center"}, {"gap", "0.75rem"},
                {"padding", "0.6rem 0.25rem"}, {"border-bottom", "1px solid #374151"},
            });
            ComponentGenerator::addChild(&row, dot("waitingDot_" + key, true));
            ComponentGenerator::addChild(&row, text("waitingText_" + key,
                "\"" + truncate(owed.text(), 110) + "\"" + kMiddot + "in " + truncate(titleOf(*t), 60),
                {{"color", "#e5e7eb"}, {"font-size", "0.88rem"}, {"font-weight", "400"}, {"margin", "0"},
                 {"flex", "1 1 auto"}, {"min-width", "0"}, {"line-height", "1.4"}}));
            ComponentGenerator::addChild(&row, ComponentGenerator::createButton(
                "answerChallenge_" + key, "Answer", "", "bg-blue-600", "hover:bg-blue-700", "text-white",
                "px-3 py-1", "rounded", "text-xs"));
            ComponentGenerator::addChild(&waiting, row);
            ++n;
        }
    }
    if (n == 0) {
        ComponentGenerator::addChild(&waiting, text("waitingEmpty", "Nothing is waiting on you.", kMuted));
    }
    ComponentGenerator::addChild(page, waiting);

    ui::Component member = section("yourDebates", "Your debates");
    int shown = 0;
    for (const Topic* t : mine) {
        if (!t->viewer_is_member()) continue;
        ComponentGenerator::addChild(&member, caseRow("mine", *t, now, true, canDelete(*t)));
        ++shown;
    }
    if (shown == 0) {
        ComponentGenerator::addChild(&member, text("yourDebatesEmpty",
            "You're not in any debates yet. Explore one and make a move.", kMuted));
    }
    ComponentGenerator::addChild(page, member);
}

// "Start a debate", at the bottom of every tab. A visitor's is one press
// (an empty CREATE_DEBATE: the sign-in modal, then a no-op replay). A
// signed-in user's opens, on the client, the create forms: a topic, or a
// passage for the AI. It stays open while a generation is running or has
// something to say.
ui::Component startDebate(const rendering_info::HomePageRenderingInfo& info, bool visitor) {
    if (visitor) {
        return linkButton("startDebateButton", "Start a debate");
    }
    const user_engagement::AiJobStatus& aiJob = info.ai_job();
    const bool busy = aiJob.pending() || !aiJob.error().empty() || !aiJob.note().empty();
    ui::Component fold = box("startDebateFold", {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.75rem"}});
    ComponentGenerator::addAttribute(&fold, "data-fold", busy ? "open" : "closed");
    ComponentGenerator::addAttribute(&fold, "data-fold-title", "Start a debate");

    // Submit topic container (inline layout)
    ui::Component submitTopicContainer = ComponentGenerator::createContainer(
        "submitTopicContainer", "flex gap-2 w-full max-w-md", "", "", "", "", "", "");
    ComponentGenerator::addChild(&submitTopicContainer, ComponentGenerator::createInput(
        "topicInput", "Enter a debate topic...", "topic", "bg-gray-800", "text-white", "border-gray-600",
        "p-3", "rounded", "flex-1"));
    ComponentGenerator::addChild(&submitTopicContainer, ComponentGenerator::createButton(
        "submitButton", "Submit", "submit", "bg-green-600", "hover:bg-green-500", "text-white",
        "px-4 py-2", "rounded-lg", "transition"));
    ComponentGenerator::addChild(&fold, submitTopicContainer);

    // ---- Build a debate from a passage, with the AI -----------------
    // Beside the manual topic box rather than replacing it: typing a topic
    // and arguing it out yourself is still the main way in, and this is the
    // shortcut for when you already have the source material.
    ui::Component generateContainer = ComponentGenerator::createContainer(
        "generateDebateContainer", "w-full max-w-md", "bg-gray-800", "p-4", "",
        "border border-gray-700", "rounded-lg", "");
    css(&generateContainer, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.5rem"}});
    ComponentGenerator::addChild(&generateContainer, text("generateHeading",
        "Or build one from a passage with AI:", {{"color", "#9ca3af"}, {"font-size", "0.8rem"},
        {"font-weight", "600"}, {"margin", "0"}}));
    ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createInput(
        "generateTopicInput", "What should the debate be about?", "topic",
        "bg-gray-900", "text-white", "border-gray-600", "p-3", "rounded", "w-full"));
    ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createTextarea(
        "generatePassageInput",
        "Paste an article or passage. The AI only uses what is written here.",
        "passage", 8));

    if (aiJob.pending()) {
        // Marker id ends in SummaryPending so the existing poll picks it up
        // -- the client polls on the WORK, and this is more of it.
        ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createText(
            "generationSummaryPending",
            "Reading the passage and building the debate... " +
                std::to_string(aiJob.elapsed_seconds()) + "s elapsed",
            "text-sm", "text-indigo-300", "", ""));
        ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createButton(
            "cancelAiWorkButton", "Cancel", "", "bg-gray-700", "hover:bg-gray-600",
            "text-gray-200", "px-3 py-1.5", "rounded", "text-sm"));
        ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createText(
            "generationPendingHint",
            "This usually takes a minute or two. The debate appears in your list when it is done.",
            "text-xs", "text-gray-500", "", ""));
    } else {
        ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createButton(
            "generateDebateButton", "Generate debate", "", "bg-indigo-600",
            "hover:bg-indigo-700", "text-white", "px-4 py-2", "rounded-lg", ""));
    }
    if (!aiJob.error().empty()) {
        ui::Component err = ComponentGenerator::createText(
            "generationError", aiJob.error(), "text-sm", "text-red-400", "", "");
        (*err.mutable_css())["overflow-wrap"] = "anywhere";
        ComponentGenerator::addChild(&generateContainer, err);
    }
    if (!aiJob.note().empty()) {
        ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createText(
            "generationNote", aiJob.note(), "text-sm", "text-green-400", "", ""));
        ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createButton(
            "dismissAiNoticeButton", "Dismiss", "", "bg-gray-700", "hover:bg-gray-600",
            "text-gray-200", "px-3 py-1.5", "rounded", "text-sm"));
    }
    ComponentGenerator::addChild(&fold, generateContainer);
    return fold;
}

}  // namespace

// The tab to draw: "home", "explore" or "mine". "Your debates" is a signed-in
// reader's only; anything else is Home.
std::string HomePageGenerator::ActiveTab(const user_engagement::HomeView& view, bool visitor) {
    if (view.home_tab() == "explore") return "explore";
    if (view.home_tab() == "mine" && !visitor) return "mine";
    return "home";
}

ui::Page HomePageGenerator::GenerateHomePage(const rendering_info::HomePageRenderingInfo& info) {
    ui::Page page;
    page.set_page_id("home");
    // A developer's title bar; LayoutGenerator drops it unless DEBATE_DEV_CHROME=1.
    page.set_title("Debate Topic Entry | commit: " GIT_COMMIT_HASH);

    ui::Component main = ComponentGenerator::createContainer(
        "main", "min-h-screen flex flex-col items-center gap-6", "bg-gray-900", "p-8", "", "", "", "text-white");

    // The site's name, the tabs and Sign in / Sign out are on the header,
    // which LayoutGenerator puts above every page.
    ui::Component column = box("homeColumn", {
        {"display", "flex"}, {"flex-direction", "column"}, {"gap", "1.75rem"},
        {"max-width", "48rem"}, {"overflow", "visible"},
    });

    const bool visitor = anonymous_user::IsAnonymousUsername(info.viewer_username());
    const std::string tab = ActiveTab(info.home_view(), visitor);
    const long long now = static_cast<long long>(std::time(nullptr));

    std::vector<const Topic*> topics;
    for (const auto& topic : info.available_debates()) {
        topics.push_back(&topic);
    }

    // Delete is the creator's alone, and never a visitor's.
    const bool mayDelete = info.can_create_or_join_debates() && !visitor;
    auto canDelete = [&](const Topic& topic) {
        return mayDelete && !topic.is_challenge() && info.viewer_user_id() > 0 &&
               topic.creator_id() == info.viewer_user_id();
    };

    if (tab == "explore") {
        addExploreTab(&column, topics, info, now, canDelete);
    } else if (tab == "mine") {
        addMineTab(&column, topics, now, canDelete);
    } else {
        addHomeTab(&column, topics, info, now);
    }

    if (info.can_create_or_join_debates()) {
        ComponentGenerator::addChild(&column, startDebate(info, visitor));
    }

    // The build, small, at the foot of the page.
    ComponentGenerator::addChild(&column, text("homeFooter", "commit " GIT_COMMIT_HASH,
        {{"color", "#4b5563"}, {"font-size", "11px"}, {"font-weight", "400"}, {"margin", "1rem 0 0"},
         {"text-align", "center"}}));

    ComponentGenerator::addChild(&main, column);
    *page.add_components() = main;
    return page;
}
