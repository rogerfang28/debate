#include "HomePageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../utils/DemoMode.h"

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

    const bool autoLogin = demo_mode::autoLogin;
    if (!autoLogin) {
        // Logout button in top right
        ui::Component logoutButton = ComponentGenerator::createButton(
            "logoutButton",
            "Logout",
            "logout",
            "bg-gray-700",
            "hover:bg-gray-600",
            "text-white",
            "px-4 py-2",
            "rounded-lg",
            "absolute top-4 right-4 transition border border-gray-600"
        );
        ComponentGenerator::addChild(&main, logoutButton);
    }

    // Title
    ui::Component title = ComponentGenerator::createText(
        "title",
        "Debate Hub",
        "text-3xl",
        "text-blue-400",
        "font-bold",
        ""
    );
    ComponentGenerator::addChild(&main, title);

    // Description
    ui::Component description = ComponentGenerator::createText(
        "description",
        "Participate in debates by creating your own and proving your point, or joining existing ones.",
        "",
        "text-gray-300",
        "",
        "text-center"
    );
    ComponentGenerator::addChild(&main, description);

    if (info.can_create_or_join_debates()) {
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

        // Join debate by ID container (inline layout)
        ui::Component joinDebateContainer = ComponentGenerator::createContainer(
            "joinDebateContainer",
            "flex gap-2 w-full max-w-md",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        // Join debate input field
        ui::Component joinDebateInput = ComponentGenerator::createInput(
            "joinDebateInput",
            "Enter debate ID...",
            "debateId",
            "bg-gray-800",
            "text-white",
            "border-gray-600",
            "p-3",
            "rounded",
            "flex-1"
        );
        ComponentGenerator::addChild(&joinDebateContainer, joinDebateInput);

        // Join button
        ui::Component joinDebateButton = ComponentGenerator::createButton(
            "joinDebateButton",
            "Join",
            "joinDebate",
            "bg-blue-600",
            "hover:bg-blue-500",
            "text-white",
            "px-4 py-2",
            "rounded-lg",
            "transition"
        );
        ComponentGenerator::addChild(&joinDebateContainer, joinDebateButton);
        ComponentGenerator::addChild(&main, joinDebateContainer);
    }

    // Topics card container - will be filled by FillDebateTopics
    ui::Component topicsCard = ComponentGenerator::createContainer(
        "topicsCard",
        "w-full max-w-3xl",
        "bg-gray-800",
        "p-6",
        "",
        "border border-gray-700",
        "rounded-lg",
        ""
    );

    ui::Component topicsTitle = ComponentGenerator::createText(
        "topicsTitle",
        "Available Debates",
        "text-xl",
        "text-blue-300",
        "font-semibold",
        "mb-4"
    );
    ComponentGenerator::addChild(&topicsCard, topicsTitle);

    // Empty topics list container - this will be populated by FillDebateTopics
    ui::Component topicsList = ComponentGenerator::createContainer(
        "topicsList",
        "flex flex-col gap-3",
        "",
        "",
        "",
        "",
        "",
        ""
    );
    ComponentGenerator::addChild(&topicsCard, topicsList);
    ComponentGenerator::addChild(&main, topicsCard);

    return main;
}

ui::Component HomePageGenerator::FillDebateTopics(const rendering_info::HomePageRenderingInfo& info, ui::Component mainLayout) {
    int currentUserId = info.viewer_user_id();

    // Find the topicsList container within mainLayout
    ui::Component* topicsList = nullptr;
    for (int i = 0; i < mainLayout.children_size(); i++) {
        ui::Component* child = mainLayout.mutable_children(i);
        if (child->id() == "topicsCard") {
            // Found the topicsCard, now find topicsList inside it
            for (int j = 0; j < child->children_size(); j++) {
                ui::Component* grandchild = child->mutable_children(j);
                if (grandchild->id() == "topicsList") {
                    topicsList = grandchild;
                    break;
                }
            }
            break;
        }
    }

    if (topicsList == nullptr) {
        // If we can't find the topicsList, just return the layout unchanged
        return mainLayout;
    }

    // Populate topics list with user's debates
    for (const auto& topic : info.available_debates()) {
        const int debateRootClaimId = topic.debate_root_claim_id();
        ui::Component debateItemContainer = ComponentGenerator::createContainer(
            "debateItemContainer_" + std::to_string(debateRootClaimId),
            "flex gap-2 items-center",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        ui::Component debateItemWrapper = ComponentGenerator::createContainer(
            "debateItemWrapper_" + std::to_string(debateRootClaimId),
            "flex flex-col flex-1",
            "bg-gray-700",
            "p-4",
            "",
            "border border-gray-600",
            "rounded",
            ""
        );

        // Change from button to text for the debate topic
        ui::Component debateItem = ComponentGenerator::createText(
            "debateTopicText_" + std::to_string(debateRootClaimId),
            topic.topic(),
            "text-base",
            "text-white",
            "font-medium",
            ""
        );
        ComponentGenerator::addChild(&debateItemWrapper, debateItem);

        if (topic.is_challenge()) {
            ui::Component challengeInfo = ComponentGenerator::createText(
                "challengeInfo_" + std::to_string(debateRootClaimId),
                "This is a challenge to claim: " + topic.claim_its_challenging(),
                "text-xs",
                "text-yellow-400",
                "italic",
                "pt-1"
            );
            ComponentGenerator::addChild(&debateItemWrapper, challengeInfo);
        }

        ComponentGenerator::addChild(&debateItemContainer, debateItemWrapper);

        // Add Enter button
        ui::Component enterButton = ComponentGenerator::createButton(
            "enterDebateTopicButton_" + std::to_string(debateRootClaimId),
            "Enter",
            "selectDebate",
            "bg-blue-600",
            "hover:bg-blue-500",
            "text-white",
            "px-6 py-2",
            "rounded-lg",
            "transition border border-blue-500 font-medium"
        );
        ComponentGenerator::addAttribute(&enterButton, "data-debate-id", std::to_string(debateRootClaimId));
        ComponentGenerator::addChild(&debateItemContainer, enterButton);

        // Show delete button if user owns the debate and it's not a challenge
        // Show leave button if user doesn't own the debate
        if (!topic.is_challenge() && info.can_create_or_join_debates()) {
            bool userOwnsDebate = (currentUserId == topic.creator_id());
            
            if (userOwnsDebate) {
                ui::Component deleteDebateButton = ComponentGenerator::createButton(
                    "deleteDebateButton_" + std::to_string(debateRootClaimId),
                    "Delete",
                    "",
                    "bg-red-600",
                    "hover:bg-red-700",
                    "text-white",
                    "px-4 py-2",
                    "rounded",
                    "transition border border-red-500"
                );
                ComponentGenerator::addChild(&debateItemContainer, deleteDebateButton);
            } else {
                ui::Component leaveDebateButton = ComponentGenerator::createButton(
                    "leaveDebateButton_" + std::to_string(debateRootClaimId),
                    "Leave",
                    "",
                    "bg-orange-600",
                    "hover:bg-orange-700",
                    "text-white",
                    "px-4 py-2",
                    "rounded",
                    "transition border border-orange-500"
                );
                ComponentGenerator::addChild(&debateItemContainer, leaveDebateButton);
            }
        }

        ComponentGenerator::addChild(topicsList, debateItemContainer);
    }

    return mainLayout;
}