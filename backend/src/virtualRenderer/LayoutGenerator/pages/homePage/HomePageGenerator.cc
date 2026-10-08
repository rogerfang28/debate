#include "HomePageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../utils/DemoMode.h"
#include "../../../../utils/AnonymousUser.h"
#include "../../../../utils/TutorialLibrary.h"

#include <algorithm>
#include <utility>
#include <vector>

namespace {

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

const Rules kCaption = {
    {"color", "#9ca3af"}, {"font-size", "0.7rem"}, {"font-weight", "700"},
    {"letter-spacing", "0.08em"}, {"text-transform", "uppercase"}, {"margin", "0"},
};
const Rules kMuted = {
    {"color", "#9ca3af"}, {"font-size", "0.85rem"}, {"line-height", "1.5"}, {"margin", "0"},
};

// Cards with fewer claims than this go in the folded section at the bottom:
// mostly test debates ("test", "1", ...) and debates nobody has argued yet.
constexpr int kShelfMinClaims = 3;

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

// One debate on the shelf: what is claimed and by whom, where it stands, how
// big it is, who is in it. The whole card is one press (data-click): it
// opens the debate on its Brief tab. Delete, for the creator only, is a quiet
// control in the corner that the card's own press ignores.
ui::Component debateCard(const rendering_info::HomeDebateTopicRenderInfo& topic, bool canDelete) {
    const std::string id = std::to_string(topic.debate_root_claim_id());
    ui::Component card = ComponentGenerator::createContainer("openDebateCard_" + id, "w-full", "", "", "", "", "", "");
    ComponentGenerator::addAttribute(&card, "data-click", "true");
    css(&card, {
        {"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.45rem"},
        {"background", "#111827"}, {"border", "1px solid #374151"},
        {"border-radius", "0.75rem"}, {"padding", "1rem 1.25rem"},
    });

    // Title row: the main claim, and Delete at the far end.
    ui::Component titleRow = ComponentGenerator::createContainer("debateCardTitleRow_" + id, "w-full", "", "", "", "", "", "");
    css(&titleRow, {{"display", "flex"}, {"flex-direction", "row"}, {"justify-content", "space-between"},
                    {"align-items", "flex-start"}, {"gap", "0.75rem"}});
    const std::string title = topic.root_text().empty() ? topic.topic() : topic.root_text();
    ComponentGenerator::addChild(&titleRow, text("debateCardTitle_" + id, truncate(title, 220),
        {{"color", "#f9fafb"}, {"font-size", "1.05rem"}, {"font-weight", "700"},
         {"line-height", "1.35"}, {"margin", "0"}, {"flex", "1 1 auto"}, {"min-width", "0"}}));
    if (canDelete) {
        ui::Component del = ComponentGenerator::createButton(
            "deleteDebateButton_" + id, "Delete", "", "bg-transparent", "hover:text-red-400",
            "text-gray-500", "px-2 py-1", "rounded", "text-xs");
        css(&del, {{"background", "transparent"}, {"border", "none"}, {"flex", "0 0 auto"}});
        ComponentGenerator::addChild(&titleRow, del);
    }
    ComponentGenerator::addChild(&card, titleRow);

    if (!topic.root_holder().empty()) {
        ComponentGenerator::addChild(&card, text("debateCardWho_" + id, "Claimed by " + topic.root_holder(), kMuted));
    }

    // Where it stands, computed the way the Brief's verdict is.
    if (topic.unresolved()) {
        ComponentGenerator::addChild(&card, text("debateCardState_" + id,
            "Unresolved, hangs on: \"" + truncate(topic.hinge_text(), 110) + "\"",
            {{"color", "#fdba74"}, {"font-size", "0.85rem"}, {"line-height", "1.45"}, {"margin", "0"}}));
    } else {
        ComponentGenerator::addChild(&card, text("debateCardState_" + id, "No open challenges",
            {{"color", "#86efac"}, {"font-size", "0.85rem"}, {"margin", "0"}}));
    }

    ComponentGenerator::addChild(&card, text("debateCardCounts_" + id,
        count(topic.claim_count(), "claim", "claims") + "  \xC2\xB7  " +
        count(topic.stance_count(), "person with a stance", "people with a stance") + "  \xC2\xB7  " +
        count(topic.open_challenge_count(), "open challenge", "open challenges"),
        {{"color", "#6b7280"}, {"font-size", "0.78rem"}, {"margin", "0"}}));

    if (topic.involved_size() > 0) {
        std::string names;
        int shown = 0;
        for (const std::string& who : topic.involved()) {
            if (shown == 5 || names.size() + who.size() > 80) break;
            names += (shown > 0 ? ", " : "") + who;
            ++shown;
        }
        if (topic.involved_size() > shown) {
            names += " +" + std::to_string(topic.involved_size() - shown) + " more";
        }
        ComponentGenerator::addChild(&card, text("debateCardInvolved_" + id, "Involved: " + names,
            {{"color", "#9ca3af"}, {"font-size", "0.78rem"}, {"margin", "0"}}));
    }
    return card;
}

}  // namespace

ui::Page HomePageGenerator::GenerateHomePage(const rendering_info::HomePageRenderingInfo& info) {
    ui::Page page;
    page.set_page_id("home");
    page.set_title("Debate Topic Entry | commit: " GIT_COMMIT_HASH);

    // Main container
    ui::Component main = GenerateHomePageMainLayout(info);

    main = FillDebateTopics(info, main);
    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(main);

    return page;
}

ui::Component HomePageGenerator::GenerateHomePageMainLayout(const rendering_info::HomePageRenderingInfo& info) {
    // Main container
    ui::Component main = ComponentGenerator::createContainer(
        "main",
        "min-h-screen flex flex-col items-center gap-6",
        "bg-gray-900",
        "p-8",
        "",
        "",
        "",
        "text-white"
    );

    // The site's name and Sign in / Sign out are on the header, which
    // LayoutGenerator puts above every page.

    // Description
    ui::Component description = ComponentGenerator::createText(
        "description",
        "Each card is one debate: its main claim, where it stands, and who is in it. Open one to read its brief.",
        "",
        "text-gray-300",
        "",
        "text-center"
    );
    ComponentGenerator::addChild(&main, description);

    const bool visitor = anonymous_user::IsAnonymousUsername(info.viewer_username());

    // ---- Guided walkthroughs ----------------------------------------
    // First thing under the description, because the tool's central idea --
    // that an argument is a graph of claims, not a thread of replies -- is not
    // guessable from an empty page with a topic box on it.
    //
    // One button per file in backend/tutorials/. Nothing here names a
    // particular walkthrough, so adding one is a file and no code.
    if (info.can_create_or_join_debates()) {
        const std::vector<tutorials::Listing> walkthroughs = tutorials::List();
        if (!walkthroughs.empty()) {
            ui::Component tutorialContainer = ComponentGenerator::createContainer(
                "tutorialContainer", "w-full max-w-md", "bg-gray-800", "p-4", "",
                "border border-blue-700", "rounded-lg", ""
            );
            (*tutorialContainer.mutable_css())["display"] = "flex";
            (*tutorialContainer.mutable_css())["flex-direction"] = "column";
            (*tutorialContainer.mutable_css())["gap"] = "0.5rem";

            ComponentGenerator::addChild(&tutorialContainer, ComponentGenerator::createText(
                "tutorialHeading", "New here?", "text-xs",
                "text-gray-500", "font-semibold uppercase tracking-wide", ""));

            for (const tutorials::Listing& walkthrough : walkthroughs) {
                ComponentGenerator::addChild(&tutorialContainer, ComponentGenerator::createButton(
                    "startTutorialButton_" + walkthrough.name,
                    walkthrough.title, "", "bg-blue-600", "hover:bg-blue-700", "text-white",
                    "px-4 py-2", "rounded-lg", "transition w-full"));
                if (!walkthrough.subtitle.empty()) {
                    ComponentGenerator::addChild(&tutorialContainer, ComponentGenerator::createText(
                        "tutorialSubtitle_" + walkthrough.name, walkthrough.subtitle,
                        "text-xs", "text-gray-400", "", "text-center"));
                }
            }
            ComponentGenerator::addChild(&main, tutorialContainer);
        }
    }

    // A visitor gets one button instead of the forms: it opens the sign-in
    // modal ("Sign in to start a debate"), and once they are signed in they
    // are back here with the forms below.
    if (info.can_create_or_join_debates() && visitor) {
        ComponentGenerator::addChild(&main, ComponentGenerator::createButton(
            "startDebateButton", "Start a debate", "", "bg-green-600", "hover:bg-green-500",
            "text-white", "px-5 py-2", "rounded-lg", "transition"));
    }

    if (info.can_create_or_join_debates() && !visitor) {
        // Submit topic container (inline layout)
        ui::Component submitTopicContainer = ComponentGenerator::createContainer(
            "submitTopicContainer",
            "flex gap-2 w-full max-w-md",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        // Input field
        ui::Component topicInput = ComponentGenerator::createInput(
            "topicInput",
            "Enter a debate topic...",
            "topic",
            "bg-gray-800",
            "text-white",
            "border-gray-600",
            "p-3",
            "rounded",
            "flex-1"
        );
        ComponentGenerator::addChild(&submitTopicContainer, topicInput);

        // Submit button
        ui::Component submitButton = ComponentGenerator::createButton(
            "submitButton",
            "Submit",
            "submit",
            "bg-green-600",
            "hover:bg-green-500",
            "text-white",
            "px-4 py-2",
            "rounded-lg",
            "transition"
        );
        ComponentGenerator::addChild(&submitTopicContainer, submitButton);
        ComponentGenerator::addChild(&main, submitTopicContainer);

        // ---- Build a debate from a passage, with the AI -----------------
        // Sits beside the manual topic box rather than replacing it: typing a
        // topic and arguing it out yourself is still the main way in, and this
        // is the shortcut for when you already have the source material.
        ui::Component generateContainer = ComponentGenerator::createContainer(
            "generateDebateContainer", "w-full max-w-md", "bg-gray-800", "p-4", "",
            "border border-gray-700", "rounded-lg", ""
        );
        (*generateContainer.mutable_css())["display"] = "flex";
        (*generateContainer.mutable_css())["flex-direction"] = "column";
        (*generateContainer.mutable_css())["gap"] = "0.5rem";
        (*generateContainer.mutable_css())["margin-top"] = "1rem";

        ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createText(
            "generateHeading", "Or build one from a passage with AI:", "text-xs", "text-gray-500",
            "font-semibold uppercase tracking-wide", ""));

        ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createInput(
            "generateTopicInput", "What should the debate be about?", "topic",
            "bg-gray-900", "text-white", "border-gray-600", "p-3", "rounded", "w-full"));

        ComponentGenerator::addChild(&generateContainer, ComponentGenerator::createTextarea(
            "generatePassageInput",
            "Paste an article or passage. The AI only uses what is written here.",
            "passage", 8));

        const user_engagement::AiJobStatus& aiJob = info.ai_job();
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
                "generateDebateButton", "Generate Debate", "", "bg-indigo-600",
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

        ComponentGenerator::addChild(&main, generateContainer);
    }

    // The shelf - filled by FillDebateTopics.
    ui::Component shelf = ComponentGenerator::createContainer("debateShelf", "w-full max-w-3xl", "", "", "", "", "", "");
    css(&shelf, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.75rem"}, {"overflow", "visible"}});
    ComponentGenerator::addChild(&shelf, text("debateShelfCaption", "Debates", kCaption));
    ComponentGenerator::addChild(&main, shelf);

    return main;
}

ui::Component HomePageGenerator::FillDebateTopics(const rendering_info::HomePageRenderingInfo& info, ui::Component mainLayout) {
    ui::Component* shelf = nullptr;
    for (int i = 0; i < mainLayout.children_size(); i++) {
        if (mainLayout.children(i).id() == "debateShelf") {
            shelf = mainLayout.mutable_children(i);
            break;
        }
    }
    if (shelf == nullptr) {
        return mainLayout;
    }

    // Biggest first; the newest first among equals.
    std::vector<const rendering_info::HomeDebateTopicRenderInfo*> topics;
    for (const auto& topic : info.available_debates()) {
        topics.push_back(&topic);
    }
    std::stable_sort(topics.begin(), topics.end(), [](const auto* a, const auto* b) {
        if (a->claim_count() != b->claim_count()) return a->claim_count() > b->claim_count();
        return a->debate_root_claim_id() > b->debate_root_claim_id();
    });

    // Delete is the creator's alone, and never a visitor's.
    const bool mayDelete = info.can_create_or_join_debates() &&
                           !anonymous_user::IsAnonymousUsername(info.viewer_username());
    auto canDelete = [&](const rendering_info::HomeDebateTopicRenderInfo& topic) {
        return mayDelete && !topic.is_challenge() && info.viewer_user_id() > 0 &&
               topic.creator_id() == info.viewer_user_id();
    };

    // The small ones fold away at the bottom. The client opens and closes the
    // fold itself (data-fold); the server only says what is in it.
    int smallCount = 0;
    for (const auto* topic : topics) {
        if (topic->claim_count() < kShelfMinClaims) ++smallCount;
    }
    ui::Component fold = ComponentGenerator::createContainer("smallDebatesFold", "w-full", "", "", "", "", "", "");
    ComponentGenerator::addAttribute(&fold, "data-fold", "closed");
    ComponentGenerator::addAttribute(&fold, "data-fold-title",
        "Smaller and test debates (" + std::to_string(smallCount) + ")");
    css(&fold, {{"display", "flex"}, {"flex-direction", "column"}, {"gap", "0.75rem"}});

    for (const auto* topic : topics) {
        ui::Component card = debateCard(*topic, canDelete(*topic));
        if (topic->claim_count() < kShelfMinClaims) {
            ComponentGenerator::addChild(&fold, card);
        } else {
            ComponentGenerator::addChild(shelf, card);
        }
    }

    if (topics.empty()) {
        ComponentGenerator::addChild(shelf, text("debateShelfEmpty", "No debates yet.", kMuted));
    }
    if (smallCount > 0) {
        ComponentGenerator::addChild(shelf, fold);
    }

    return mainLayout;
}
