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

// The theme's tokens (index.css), so the Brief is painted from the same
// palette as every other page: captions in text 3, prose in text 2.
const Rules kCaption = {
    {"color", "var(--text-3)"}, {"font-size", "12px"}, {"font-weight", "600"},
    {"letter-spacing", "0.06em"}, {"text-transform", "uppercase"}, {"margin", "0"},
};
const Rules kBody = {
    {"color", "var(--text-2)"}, {"font-size", "14px"}, {"line-height", "1.6"}, {"margin", "0"},
};
const Rules kMuted = {
    {"color", "var(--text-3)"}, {"font-size", "13px"}, {"line-height", "1.5"}, {"margin", "0"},
};

// A bordered block with a caption. Every section of the page is one.
ui::Component card(const std::string& id, const std::string& caption) {
    ui::Component c = ComponentGenerator::createContainer(id, "w-full", "", "", "", "", "", "");
    css(&c, {
        {"display", "flex"}, {"flex-direction", "column"}, {"gap", "8px"},
        {"background", "var(--panel)"},
        // Each side separately: a hinge card overrides border-left, and a
        // "border" shorthand in the same unordered map could flatten it.
        {"border-top", "1px solid var(--line)"}, {"border-right", "1px solid var(--line)"},
        {"border-bottom", "1px solid var(--line)"}, {"border-left", "1px solid var(--line)"},
        {"border-radius", "var(--radius)"}, {"padding", "16px 18px"},
    });
    if (!caption.empty()) {
        ComponentGenerator::addChild(&c, text(id + "Caption", caption, kCaption));
    }
    return c;
}

// A small round status dot (theme.html .dot): red while someone owes a
// reply, green when answered, grey otherwise.
ui::Component dot(const std::string& id, const std::string& color) {
    ui::Component d = ComponentGenerator::createContainer(id, "", "", "", "", "", "", "");
    css(&d, {{"width", "8px"}, {"height", "8px"}, {"min-width", "8px"}, {"border-radius", "9999px"},
             {"background", color}, {"flex", "0 0 auto"}, {"display", "block"}, {"gap", "0"}});
    return d;
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
    // A document: one scrolling reading column with the chat beside it. The
    // server says which part is which; the client lays them out.
    ComponentGenerator::addAttribute(&container, "data-layout", "document");

    // ---- Top bar: the tabs, under the site header (which has the way home) ----
    ui::Component topBar = ComponentGenerator::createContainer("briefTopBar", "w-full", "", "", "", "", "", "");
    css(&topBar, {{"display", "flex"}, {"flex-direction", "row"}, {"justify-content", "space-between"},
                  {"align-items", "center"}, {"flex-wrap", "wrap"}, {"gap", "0.5rem"}});
    ComponentGenerator::addChild(&topBar, StepView::BuildDebateTabs(true));
    ComponentGenerator::addChild(&container, topBar);

    // The brief on the left, the chat region on the right. The column is the
    // page region; its css is the reading column's (820px, as shell.html).
    ui::Component contentRow = ComponentGenerator::createContainer("briefContentRow", "w-full", "", "", "", "", "", "");
    ui::Component column = ComponentGenerator::createContainer("briefColumn", "", "", "", "", "", "", "");
    ComponentGenerator::addAttribute(&column, "data-region", "page");
    css(&column, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "14px"},
                  {"max-width", "820px"}});

    // ---- Title ------------------------------------------------------------------
    {
        ui::Component title = ComponentGenerator::createContainer("briefTitle", "w-full", "", "", "", "", "", "");
        css(&title, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.25rem"}});
        ComponentGenerator::addChild(&title, text("briefTitleCaption", "The brief", kCaption));
        ComponentGenerator::addChild(&title, text("briefTitleText", facts.rootText,
            {{"color", "var(--text)"}, {"font-size", "24px"}, {"font-weight", "650"},
             {"line-height", "1.25"}, {"letter-spacing", "-0.01em"}, {"margin", "4px 0 0"}}));
        if (!facts.rootHolder.empty()) {
            ComponentGenerator::addChild(&title, text("briefTitleWho", "Claimed by " + facts.rootHolder + ".", kMuted));
        }
        ComponentGenerator::addChild(&column, title);
    }

    // ---- Verdict ----------------------------------------------------------------
    {
        ui::Component verdict = card("briefVerdict", "");
        css(&verdict, {{"border-left", "3px solid " + std::string(facts.openByHolder.empty() ? "var(--settled)" : "var(--open)")}});
        int openSites = 0;
        const DebateBrief::Site* hinge = nullptr;
        for (const DebateBrief::Site& s : facts.sites) {
            if (s.open) ++openSites;
            if (s.hinge) hinge = &s;
        }
        ComponentGenerator::addChild(&verdict, text("briefVerdictPill",
            openSites > 0 ? "Unresolved" : "No open challenges",
            {{"color", openSites > 0 ? "var(--open)" : "var(--settled)"}, {"font-size", "12px"},
             {"font-weight", "600"}, {"letter-spacing", "0.06em"}, {"text-transform", "uppercase"}, {"margin", "0"}}));

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
            {{"color", "var(--text)"}, {"font-size", "16px"}, {"line-height", "1.55"}, {"font-weight", "400"}, {"margin", "0"}}));

        // The model's part: asked for, in progress, or behind the debate.
        ui::Component aiRow = ComponentGenerator::createContainer("briefAiRow", "w-full", "", "", "", "", "", "");
        css(&aiRow, {{"display", "flex"}, {"flex-direction", "row"}, {"flex-wrap", "wrap"},
                     {"align-items", "center"}, {"gap", "0.5rem"}, {"margin-top", "0.25rem"}});
        if (state.pending()) {
            // The id ends in Pending so the client polls while it writes.
            ComponentGenerator::addChild(&aiRow, text("briefPending",
                "Writing the words... " + std::to_string(state.elapsed_seconds()) + "s elapsed",
                {{"color", "var(--accent-hover)"}, {"font-size", "13px"}, {"margin", "0"}}));
            ComponentGenerator::addChild(&aiRow, smallButton("cancelAiWorkButton", "Cancel"));
        } else {
            ComponentGenerator::addChild(&aiRow, smallButton("generateBriefButton",
                haveWords ? "Rewrite the words with AI" : "Write the words with AI"));
            if (haveWords && !wordsCurrent) {
                ComponentGenerator::addChild(&aiRow, text("briefStale",
                    "The debate has changed since the words were written; the claims' own sentences are shown instead.",
                    {{"color", "var(--warn)"}, {"font-size", "13px"}, {"margin", "0"}}));
            }
            if (!state.error().empty()) {
                ComponentGenerator::addChild(&aiRow, text("briefError", state.error(),
                    {{"color", "var(--open)"}, {"font-size", "13px"}, {"margin", "0"}}));
            }
        }
        ComponentGenerator::addChild(&verdict, aiRow);
        ComponentGenerator::addChild(&verdict, text("briefAiNote",
            "Who holds what, which challenges are open and whose move it is are computed from the debate. "
            "Only the verdict sentence, the story and the site names are written by the model.",
            {{"color", "var(--text-3)"}, {"font-size", "12.5px"}, {"line-height", "1.4"}, {"margin", "0"}}));
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
                {{"color", color}, {"font-size", "13px"}, {"font-weight", "500"}, {"margin", "0"}, {"min-width", "8rem"}}));
            ComponentGenerator::addChild(&r, text(id + "Names", names + ".", kBody));
            ComponentGenerator::addChild(&sides, r);
        };
        row("briefSidesTrue", "Say it is true:", trueNames, "var(--settled)");
        row("briefSidesFalse", "Say it is false:", falseNames, "var(--open)");
        row("briefSidesUndecided", "Undecided:", undecided, "var(--text-3)");
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
                css(&block, {{"border-left", "3px solid var(--open)"}});
            }

            std::string name;
            auto named = prose.siteNames.find(site.id);
            if (wordsCurrent && named != prose.siteNames.end() && !named->second.empty()) {
                name = named->second;
            } else if (!site.moves.empty()) {
                name = site.moves.front().text;
            }
            ComponentGenerator::addChild(&block, text("briefSiteName_" + sid, std::to_string(number) + ". " + name,
                {{"color", "var(--text)"}, {"font-size", "15px"}, {"font-weight", "600"}, {"line-height", "1.4"}, {"margin", "0"}}));
            if (site.hinge) {
                ComponentGenerator::addChild(&block, text("briefSiteHinge_" + sid,
                    "The whole case hangs on this one.",
                    {{"color", "var(--warn)"}, {"font-size", "13px"}, {"font-weight", "400"}, {"margin", "0"}}));
            }

            // The exchange. The first line is the claim; then each challenge
            // to it, and each reply to a challenge.
            ui::Component exchange = ComponentGenerator::createContainer("briefSiteMoves_" + sid, "w-full", "", "", "", "", "", "");
            css(&exchange, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.35rem"},
                            {"border-left", "2px solid var(--line-strong)"}, {"padding-left", "12px"}});
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
                    {{"color", m.challenge ? "var(--text)" : "#c7d2fe"}, {"font-size", "13.5px"},
                     {"line-height", "1.5"}, {"margin", "0"}, {"overflow-wrap", "anywhere"}}));
                ++moveIndex;
            }
            ComponentGenerator::addChild(&block, exchange);

            // The dot, and one sentence.
            std::string dotWords, dotColor;
            if (site.open) {
                dotWords = (site.turn == viewerName ? "You have" : site.turn + (site.turn.find(',') == std::string::npos ? " has" : " have"))
                           + " not replied.";
                dotColor = "var(--open)";
            } else if (site.outcome.find("conceded") != std::string::npos) {
                dotWords = site.outcome + ".";
                dotColor = "var(--text-3)";
            } else {
                dotWords = "Answered.";
                dotColor = "var(--settled)";
            }
            ui::Component footer = ComponentGenerator::createContainer("briefSiteFooter_" + sid, "w-full", "", "", "", "", "", "");
            css(&footer, {{"display", "flex"}, {"flex-direction", "row"}, {"justify-content", "space-between"},
                          {"align-items", "center"}, {"flex-wrap", "wrap"}, {"gap", "0.5rem"}});
            ui::Component status = ComponentGenerator::createContainer("briefSiteStatusLine_" + sid, "", "", "", "", "", "", "");
            css(&status, {{"display", "flex"}, {"flex-direction", "row"}, {"align-items", "center"}, {"gap", "8px"}});
            ComponentGenerator::addChild(&status, dot("briefSiteDot_" + sid, dotColor));
            ComponentGenerator::addChild(&status, text("briefSiteStatus_" + sid, dotWords,
                {{"color", dotColor}, {"font-size", "14px"}, {"font-weight", "500"}, {"margin", "0"}}));
            ComponentGenerator::addChild(&footer, status);
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
                    {{"color", isViewer ? "var(--open)" : "var(--text)"}, {"font-size", "14px"}, {"font-weight", "500"}, {"margin", "0"}}));
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
                {{"color", m.challenge ? "var(--text-2)" : "#c7d2fe"}, {"font-size", "13.5px"},
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
                {{"color", "var(--accent)"}, {"font-size", "13px"}, {"margin", "0"}, {"overflow-wrap", "anywhere"}}));
        }
        ComponentGenerator::addChild(&column, sources);
    }

    ComponentGenerator::addChild(&contentRow, column);

    // The chat beside the brief, so "explain site 3" is one question away here too.
    int currentClaimId = fullDebateInfo.viewer_current_claim_id();
    if (currentClaimId <= 0 || collectionProto.claims_by_id().find(currentClaimId) == collectionProto.claims_by_id().end()) {
        currentClaimId = facts.rootClaimId;
    }
    ComponentGenerator::addChild(&contentRow, StepView::BuildChatColumn(viewerUser, currentClaimId));
    ComponentGenerator::addChild(&container, contentRow);

    ui::Component* root = page.add_components();
    root->CopyFrom(container);
    return page;
}
