#include "BriefView.h"

#include "../StepView/StepView.h"
#include "../../../ComponentGenerator.h"
#include "../../../../../database/virtualrenderer/VRUserDatabase.h"
#include "../../../../../utils/DebateBrief.h"

#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace {

using Rules = std::vector<std::pair<std::string, std::string>>;

void css(ui::Component* component, const Rules& rules) {
    for (const auto& rule : rules) {
        (*component->mutable_css())[rule.first] = rule.second;
    }
}

// A TEXT with real CSS. TextComponent restyles anything it is not told
// explicitly, so the words here always say what they want.
ui::Component text(const std::string& id, const std::string& words, const Rules& rules) {
    ui::Component component = ComponentGenerator::createText(id, words, "", "", "", "");
    css(&component, rules);
    return component;
}

const Rules kCaption = {
    {"color", "#9ca3af"}, {"font-size", "0.7rem"}, {"font-weight", "700"},
    {"letter-spacing", "0.08em"}, {"text-transform", "uppercase"}, {"margin", "0"},
};
const Rules kBody = {
    {"color", "#e5e7eb"}, {"font-size", "0.95rem"}, {"line-height", "1.6"}, {"margin", "0"},
};
const Rules kMuted = {
    {"color", "#9ca3af"}, {"font-size", "0.85rem"}, {"line-height", "1.5"}, {"margin", "0"},
};

// A bordered block with a caption. Every section of the page is one.
ui::Component card(const std::string& id, const std::string& caption) {
    ui::Component c = ComponentGenerator::createContainer(id, "w-full", "", "", "", "", "", "");
    css(&c, {
        {"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.6rem"},
        {"background", "#111827"}, {"border", "1px solid #374151"},
        {"border-radius", "0.75rem"}, {"padding", "1rem 1.25rem"},
    });
    if (!caption.empty()) {
        ComponentGenerator::addChild(&c, text(id + "Caption", caption, kCaption));
    }
    return c;
}

// One block of prose, one TEXT per line so paragraphs survive.
void addLines(ui::Component* parent, const std::string& idPrefix, const std::string& words,
              const Rules& rules) {
    std::istringstream lines(words);
    std::string line;
    int index = 0;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        if (line.empty()) continue;
        ComponentGenerator::addChild(parent, text(idPrefix + "_" + std::to_string(index++), line, rules));
    }
}

ui::Component smallButton(const std::string& id, const std::string& label) {
    ui::Component b = ComponentGenerator::createButton(
        id, label, "", "bg-gray-700", "hover:bg-gray-600", "text-gray-200", "px-3 py-1.5",
        "rounded", "text-xs");
    css(&b, {{"align-self", "flex-start"}});
    return b;
}

}  // namespace

ui::Page BriefView::GenerateBriefPage(
    const rendering_info::FullDebateViewInfo& fullDebateInfo,
    const debate::Collection& collectionProto,
    VRUserDatabase& userDb,
    const user::User& viewerUser
) {
    ui::Page page;
    page.set_page_id("debate");
    page.set_title("commit: " GIT_COMMIT_HASH);

    const std::string viewerName = fullDebateInfo.viewer_username();
    auto nameOf = [&](int id) -> std::string { return id > 0 ? userDb.getUsername(id) : std::string(); };
    const DebateBrief::Facts facts = DebateBrief::Compute(collectionProto, nameOf);
    const std::string fingerprint = DebateBrief::Fingerprint(facts);

    // The words, if the model has written them for THIS structure.
    const user_engagement::DebatingInfo::BriefState& state = viewerUser.engagement().debating_info().brief();
    DebateBrief::Prose prose;
    const bool haveWords = DebateBrief::DeserializeProse(state.prose_json(), &prose);
    const bool wordsCurrent = haveWords && state.prose_fingerprint() == fingerprint;

    ui::Component container = ComponentGenerator::createContainer(
        "briefMain", "min-h-screen flex flex-col items-stretch", "bg-gray-900", "p-6", "", "", "", "gap-4");
    (*container.mutable_css())["overflow"] = "visible";

    // ---- Top bar: the tabs, and Home ----------------------------------------
    ui::Component topBar = ComponentGenerator::createContainer("briefTopBar", "w-full", "", "", "", "", "", "");
    css(&topBar, {{"display", "flex"}, {"flex-direction", "row"}, {"justify-content", "space-between"},
                  {"align-items", "center"}, {"flex-wrap", "wrap"}, {"gap", "0.5rem"}});
    ComponentGenerator::addChild(&topBar, StepView::BuildDebateTabs(true));
    ComponentGenerator::addChild(&topBar, ComponentGenerator::createButton(
        "goHomeButton", "Home", "", "bg-blue-600", "hover:bg-blue-700", "text-white", "px-4 py-2",
        "rounded", "transition-colors text-sm"));
    ComponentGenerator::addChild(&container, topBar);

    // One readable column.
    ui::Component column = ComponentGenerator::createContainer("briefColumn", "w-full", "", "", "", "", "", "");
    // Left-aligned, not centred: the chat is docked to the right-hand
    // corner, and a centred column on a wide screen would sit under it.
    css(&column, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "1rem"},
                  {"max-width", "56rem"}, {"width", "100%"}, {"margin", "0"}});

    // ---- Title ------------------------------------------------------------------
    {
        ui::Component title = ComponentGenerator::createContainer("briefTitle", "w-full", "", "", "", "", "", "");
        css(&title, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.25rem"}});
        ComponentGenerator::addChild(&title, text("briefTitleCaption", "The brief", kCaption));
        ComponentGenerator::addChild(&title, text("briefTitleText", facts.rootText,
            {{"color", "#f9fafb"}, {"font-size", "1.4rem"}, {"font-weight", "700"},
             {"line-height", "1.3"}, {"margin", "0"}}));
        if (!facts.rootHolder.empty()) {
            ComponentGenerator::addChild(&title, text("briefTitleWho", "Claimed by " + facts.rootHolder + ".", kMuted));
        }
        ComponentGenerator::addChild(&column, title);
    }

    // ---- Verdict ----------------------------------------------------------------
    {
        ui::Component verdict = card("briefVerdict", "");
        css(&verdict, {{"border-left", "5px solid " + std::string(facts.openByHolder.empty() ? "#22c55e" : "#f97316")}});
        int openSites = 0;
        const DebateBrief::Site* hinge = nullptr;
        for (const DebateBrief::Site& s : facts.sites) {
            if (s.open) ++openSites;
            if (s.hinge) hinge = &s;
        }
        ComponentGenerator::addChild(&verdict, text("briefVerdictPill",
            openSites > 0 ? "Unresolved" : "No open challenges",
            {{"color", openSites > 0 ? "#fdba74" : "#86efac"}, {"font-size", "0.8rem"},
             {"font-weight", "800"}, {"letter-spacing", "0.08em"}, {"text-transform", "uppercase"}, {"margin", "0"}}));

        std::string verdictWords;
        if (wordsCurrent && !prose.verdict.empty()) {
            verdictWords = prose.verdict;
        } else if (hinge) {
            const bool several = hinge->turn.find(',') != std::string::npos;
            verdictWords = "It hangs on this claim: \"" + (hinge->moves.empty() ? std::string() : hinge->moves.front().text) +
                           "\" It has been challenged, and " + hinge->turn + (several ? " have" : " has") + " not replied.";
        } else {
            verdictWords = "Nothing in this debate is currently challenged.";
        }
        ComponentGenerator::addChild(&verdict, text("briefVerdictText", verdictWords,
            {{"color", "#f9fafb"}, {"font-size", "1.05rem"}, {"line-height", "1.55"}, {"font-weight", "500"}, {"margin", "0"}}));

        // The model's part: asked for, in progress, or behind the debate.
        ui::Component aiRow = ComponentGenerator::createContainer("briefAiRow", "w-full", "", "", "", "", "", "");
        css(&aiRow, {{"display", "flex"}, {"flex-direction", "row"}, {"flex-wrap", "wrap"},
                     {"align-items", "center"}, {"gap", "0.5rem"}, {"margin-top", "0.25rem"}});
        if (state.pending()) {
            // The id ends in Pending so the client polls while it writes.
            ComponentGenerator::addChild(&aiRow, text("briefPending",
                "Writing the words... " + std::to_string(state.elapsed_seconds()) + "s elapsed",
                {{"color", "#a5b4fc"}, {"font-size", "0.85rem"}, {"margin", "0"}}));
            ComponentGenerator::addChild(&aiRow, smallButton("cancelAiWorkButton", "Cancel"));
        } else {
            ComponentGenerator::addChild(&aiRow, smallButton("generateBriefButton",
                haveWords ? "Rewrite the words with AI" : "Write the words with AI"));
            if (haveWords && !wordsCurrent) {
                ComponentGenerator::addChild(&aiRow, text("briefStale",
                    "The debate has changed since the words were written; the claims' own sentences are shown instead.",
                    {{"color", "#fbbf24"}, {"font-size", "0.8rem"}, {"margin", "0"}}));
            }
            if (!state.error().empty()) {
                ComponentGenerator::addChild(&aiRow, text("briefError", state.error(),
                    {{"color", "#fca5a5"}, {"font-size", "0.8rem"}, {"margin", "0"}}));
            }
        }
        ComponentGenerator::addChild(&verdict, aiRow);
        ComponentGenerator::addChild(&verdict, text("briefAiNote",
            "Who holds what, which challenges are open and whose move it is are computed from the debate. "
            "Only the verdict sentence, the story and the site names are written by the model.",
            {{"color", "#6b7280"}, {"font-size", "0.75rem"}, {"line-height", "1.4"}, {"margin", "0"}}));
        ComponentGenerator::addChild(&column, verdict);
    }

    // ---- The story --------------------------------------------------------------
    {
        ui::Component story = card("briefStory", "The story");
        if (wordsCurrent && !prose.story.empty()) {
            addLines(&story, "briefStoryLine", prose.story, kBody);
        } else {
            int open = 0;
            for (const DebateBrief::Site& s : facts.sites) if (s.open) ++open;
            std::ostringstream words;
            words << facts.rootHolder << " claims: " << facts.rootText << " "
                  << facts.stances.size() << (facts.stances.size() == 1 ? " person has" : " people have")
                  << " taken a position. " << facts.sites.size()
                  << (facts.sites.size() == 1 ? " disagreement has" : " disagreements have") << " been raised, and "
                  << open << (open == 1 ? " still waits" : " still wait") << " for a reply.";
            ComponentGenerator::addChild(&story, text("briefStoryFallback", words.str(), kBody));
        }
        ComponentGenerator::addChild(&column, story);
    }

    // ---- Sides ---------------------------------------------------------------------
    {
        ui::Component sides = card("briefSides", "Sides");
        // Undecided is only worth naming for people who are PARTY to a
        // dispute -- they challenged something without taking a side on the
        // main claim. Everyone else on the undecided list is a voice quoted
        // only for evidence (a statute, an agency, a reporter), and thirty
        // of those in a row tell the reader nothing; they become a count.
        std::set<std::string> challengers;
        for (const DebateBrief::Site& site : facts.sites) {
            for (const DebateBrief::Move& m : site.moves) {
                if (m.challenge) challengers.insert(m.who);
            }
        }
        std::string trueNames, falseNames, undecided;
        int evidenceOnly = 0;
        for (const DebateBrief::Stance& s : facts.stances) {
            if (s.status == debate::UNDETERMINED && !challengers.count(s.who) && s.who != viewerName) {
                ++evidenceOnly;
                continue;
            }
            std::string& bucket = s.status == debate::TRUE_CLAIM ? trueNames
                                : s.status == debate::FALSE_CLAIM ? falseNames : undecided;
            if (!bucket.empty()) bucket += ", ";
            bucket += s.who == viewerName ? "you" : s.who;
        }
        auto row = [&](const std::string& id, const std::string& label, const std::string& names, const std::string& color) {
            if (names.empty()) return;
            ui::Component r = ComponentGenerator::createContainer(id, "", "", "", "", "", "", "");
            css(&r, {{"display", "flex"}, {"flex-direction", "row"}, {"gap", "0.6rem"}, {"align-items", "baseline"}, {"flex-wrap", "wrap"}});
            ComponentGenerator::addChild(&r, text(id + "Label", label,
                {{"color", color}, {"font-size", "0.85rem"}, {"font-weight", "700"}, {"margin", "0"}, {"min-width", "8rem"}}));
            ComponentGenerator::addChild(&r, text(id + "Names", names + ".", kBody));
            ComponentGenerator::addChild(&sides, r);
        };
        row("briefSidesTrue", "Say it is true:", trueNames, "#86efac");
        row("briefSidesFalse", "Say it is false:", falseNames, "#fca5a5");
        row("briefSidesUndecided", "Undecided:", undecided, "#d1d5db");
        if (evidenceOnly > 0) {
            ComponentGenerator::addChild(&sides, text("briefSidesEvidenceOnly",
                std::to_string(evidenceOnly) + (evidenceOnly == 1 ? " other voice is" : " other voices are") +
                    " quoted only as sources of evidence and take no side.",
                kMuted));
        }
        if (trueNames.empty() && falseNames.empty() && undecided.empty()) {
            ComponentGenerator::addChild(&sides, text("briefSidesNone", "Nobody has taken a position yet.", kMuted));
        }
        ComponentGenerator::addChild(&column, sides);
    }

    // ---- Battle sites ----------------------------------------------------------
    {
        ui::Component heading = ComponentGenerator::createContainer("briefSitesHeading", "w-full", "", "", "", "", "", "");
        css(&heading, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.2rem"}, {"margin-top", "0.5rem"}});
        ComponentGenerator::addChild(&heading, text("briefSitesCaption", "Battle sites", kCaption));
        ComponentGenerator::addChild(&heading, text("briefSitesIntro",
            facts.sites.empty() ? "Nobody has challenged anything yet."
                                : "Each disagreement as the back-and-forth it was. A red dot means someone still owes a reply.",
            kMuted));
        ComponentGenerator::addChild(&column, heading);

        int number = 0;
        for (const DebateBrief::Site& site : facts.sites) {
            ++number;
            const std::string sid = std::to_string(site.id);
            ui::Component block = card("briefSite_" + sid, "");
            if (site.hinge) {
                css(&block, {{"border-left", "5px solid #f97316"}});
            }

            std::string name;
            auto named = prose.siteNames.find(site.id);
            if (wordsCurrent && named != prose.siteNames.end() && !named->second.empty()) {
                name = named->second;
            } else if (!site.moves.empty()) {
                name = site.moves.front().text;
            }
            ComponentGenerator::addChild(&block, text("briefSiteName_" + sid, std::to_string(number) + ". " + name,
                {{"color", "#f9fafb"}, {"font-size", "1.05rem"}, {"font-weight", "700"}, {"line-height", "1.35"}, {"margin", "0"}}));
            if (site.hinge) {
                ComponentGenerator::addChild(&block, text("briefSiteHinge_" + sid,
                    "The whole case hangs on this one.",
                    {{"color", "#fdba74"}, {"font-size", "0.8rem"}, {"font-weight", "600"}, {"margin", "0"}}));
            }

            // The exchange. The first line is the claim; then each challenge
            // to it, and each reply to a challenge.
            ui::Component exchange = ComponentGenerator::createContainer("briefSiteMoves_" + sid, "w-full", "", "", "", "", "", "");
            css(&exchange, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.35rem"},
                            {"border-left", "2px solid #374151"}, {"padding-left", "0.75rem"}});
            int moveIndex = 0;
            for (const DebateBrief::Move& m : site.moves) {
                std::string verb;
                if (m.challenge) {
                    verb = m.reply ? " replied"
                         : m.againstReasoning ? " challenged the reasoning" : " challenged";
                }
                const std::string who = (m.who == viewerName ? "You" : m.who);
                ComponentGenerator::addChild(&exchange, text(
                    "briefSiteMove_" + sid + "_" + std::to_string(moveIndex),
                    who + verb + ": " + m.text,
                    {{"color", m.challenge ? "#e5e7eb" : "#c7d2fe"}, {"font-size", "0.92rem"},
                     {"line-height", "1.5"}, {"margin", "0"}, {"overflow-wrap", "anywhere"}}));
                ++moveIndex;
            }
            ComponentGenerator::addChild(&block, exchange);

            // The dot, and one sentence.
            std::string dot, dotWords, dotColor;
            if (site.open) {
                dot = "\xF0\x9F\x94\xB4 ";
                dotWords = (site.turn == viewerName ? "You have" : site.turn + (site.turn.find(',') == std::string::npos ? " has" : " have"))
                           + " not replied.";
                dotColor = "#fca5a5";
            } else if (site.outcome.find("conceded") != std::string::npos) {
                dot = "\xE2\x9A\xAA ";
                dotWords = site.outcome + ".";
                dotColor = "#d1d5db";
            } else {
                dot = "\xF0\x9F\x9F\xA2 ";
                dotWords = "Answered.";
                dotColor = "#86efac";
            }
            ui::Component footer = ComponentGenerator::createContainer("briefSiteFooter_" + sid, "w-full", "", "", "", "", "", "");
            css(&footer, {{"display", "flex"}, {"flex-direction", "row"}, {"justify-content", "space-between"},
                          {"align-items", "center"}, {"flex-wrap", "wrap"}, {"gap", "0.5rem"}});
            ComponentGenerator::addChild(&footer, text("briefSiteStatus_" + sid, dot + dotWords,
                {{"color", dotColor}, {"font-size", "0.9rem"}, {"font-weight", "600"}, {"margin", "0"}}));
            const int jumpTo = site.moves.empty() ? site.targetClaimId : site.moves.back().claimId;
            ComponentGenerator::addChild(&footer, smallButton("briefOpenClaim_" + std::to_string(jumpTo), "Open in debate"));
            ComponentGenerator::addChild(&block, footer);

            ComponentGenerator::addChild(&column, block);
        }
    }

    // ---- Common ground ----------------------------------------------------------
    {
        ui::Component common = card("briefCommon", "Common ground");
        if (facts.commonGround.empty()) {
            ComponentGenerator::addChild(&common, text("briefCommonNone",
                "Nothing yet that more than one person holds and nobody disputes.", kMuted));
        } else {
            for (const DebateBrief::Move& m : facts.commonGround) {
                ComponentGenerator::addChild(&common, text("briefCommon_" + std::to_string(m.claimId),
                    "\xE2\x80\xA2 " + m.text, kBody));
            }
        }
        ComponentGenerator::addChild(&column, common);
    }

    // ---- Who owes a reply, the viewer first ----------------------------------
    {
        ui::Component owed = card("briefOwed", "Who owes a reply");
        if (facts.openByHolder.empty()) {
            ComponentGenerator::addChild(&owed, text("briefOwedNone", "Nobody. Every challenge has been answered.", kMuted));
        } else {
            std::vector<std::string> order;
            if (facts.openByHolder.count(viewerName)) order.push_back(viewerName);
            for (const auto& entry : facts.openByHolder) {
                if (entry.first != viewerName) order.push_back(entry.first);
            }
            for (const std::string& holder : order) {
                const std::vector<int>& list = facts.openByHolder.at(holder);
                const bool isViewer = holder == viewerName;
                ComponentGenerator::addChild(&owed, text("briefOwedWho_" + holder,
                    (isViewer ? "You" : holder) + ": " + std::to_string(list.size()) +
                        (list.size() == 1 ? " challenge waiting" : " challenges waiting") +
                        (isViewer ? " for your answer" : ""),
                    {{"color", isViewer ? "#fdba74" : "#f9fafb"}, {"font-size", "0.92rem"}, {"font-weight", "700"}, {"margin", "0"}}));
                for (int challengeId : list) {
                    auto it = collectionProto.claims_by_id().find(challengeId);
                    if (it == collectionProto.claims_by_id().end()) continue;
                    ui::Component row = ComponentGenerator::createContainer(
                        "briefOwedRow_" + holder + "_" + std::to_string(challengeId), "w-full", "", "", "", "", "", "");
                    css(&row, {{"display", "flex"}, {"flex-direction", "row"}, {"justify-content", "space-between"},
                               {"align-items", "center"}, {"gap", "0.5rem"}, {"padding-left", "0.75rem"}});
                    ComponentGenerator::addChild(&row, text(
                        "briefOwedItem_" + holder + "_" + std::to_string(challengeId),
                        "\xE2\x80\xA2 " + it->second.sentence(), kBody));
                    if (isViewer) {
                        ComponentGenerator::addChild(&row, smallButton(
                            "briefOpenClaim_" + std::to_string(challengeId), "Answer it"));
                    }
                    ComponentGenerator::addChild(&owed, row);
                }
            }
        }
        ComponentGenerator::addChild(&column, owed);
    }

    // ---- How it got here -----------------------------------------------------------
    {
        ui::Component timeline = card("briefTimeline", "How it got here");
        int n = 0;
        for (const DebateBrief::Move& m : facts.timeline) {
            ++n;
            const std::string who = m.who == viewerName ? "You" : m.who;
            ComponentGenerator::addChild(&timeline, text("briefTimeline_" + std::to_string(m.claimId),
                std::to_string(n) + ". " + who + (m.challenge ? " challenged: " : " claimed: ") + m.text,
                {{"color", m.challenge ? "#e5e7eb" : "#c7d2fe"}, {"font-size", "0.88rem"},
                 {"line-height", "1.5"}, {"margin", "0"}, {"overflow-wrap", "anywhere"}}));
        }
        if (n == 0) {
            ComponentGenerator::addChild(&timeline, text("briefTimelineNone", "No claims yet.", kMuted));
        }
        ComponentGenerator::addChild(&column, timeline);
    }

    // ---- Sources ------------------------------------------------------------------
    if (!facts.sources.empty()) {
        ui::Component sources = card("briefSources", "Sources");
        int n = 0;
        for (const std::string& url : facts.sources) {
            ComponentGenerator::addChild(&sources, text("briefSource_" + std::to_string(n++), url,
                {{"color", "#93c5fd"}, {"font-size", "0.85rem"}, {"margin", "0"}, {"overflow-wrap", "anywhere"}}));
        }
        ComponentGenerator::addChild(&column, sources);
    }

    ComponentGenerator::addChild(&container, column);

    // The chat, docked, so "explain site 3" is one question away here too.
    int currentClaimId = fullDebateInfo.viewer_current_claim_id();
    if (currentClaimId <= 0 || collectionProto.claims_by_id().find(currentClaimId) == collectionProto.claims_by_id().end()) {
        currentClaimId = facts.rootClaimId;
    }
    ComponentGenerator::addChild(&container, StepView::BuildChatPanel(viewerUser, currentClaimId));

    ui::Component* root = page.add_components();
    root->CopyFrom(container);
    return page;
}
