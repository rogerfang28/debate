#include "HomePageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"

ui::Page HomePageGenerator::GenerateHomePage(user::User user) {
    user_engagement::DebateList usersDebates = user.engagement().home_info().available_debates();
    ui::Page page;
    page.set_page_id("home");
    page.set_title("Debate Topic Entry");

    // Main container
    ui::Component main = GenerateHomePageMainLayout();

    main = FillDebateTopics(user, main);
    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(main);

    return page;
}
//     // Logout button in top right
//     ui::Component logoutButton = ComponentGenerator::createButton(
//         "logoutButton",
//         "Logout",
//         "logout",
//         "bg-gray-700",
//         "hover:bg-gray-600",
//         "text-white",
//         "px-4 py-2",
//         "rounded-lg",
//         "absolute top-4 right-4 transition border border-gray-600"
//     );
//     ComponentGenerator::addChild(&main, logoutButton);

//     // Title
//     ui::Component title = ComponentGenerator::createText(
//         "title",
//         "Debate Hub",
//         "text-3xl",
//         "text-blue-400",
//         "font-bold",
//         ""
//     );
//     ComponentGenerator::addChild(&main, title);

//     // Description
//     ui::Component description = ComponentGenerator::createText(
//         "description",
//         "Participate in debates by creating your own and proving your point, or joining existing ones.",
//         "",
//         "text-gray-300",
//         "",
//         "text-center"
//     );
//     ComponentGenerator::addChild(&main, description);

//     // Clear button
//     ui::Component clearButton = ComponentGenerator::createButton(
//         "clearButton",
//         "Clear All Topics",
//         "clear",
//         "bg-red-600",
//         "hover:bg-red-500",
//         "text-white",
//         "px-4 py-2",
//         "rounded-lg",
//         "transition"
//     );
//     // ComponentGenerator::addChild(&main, clearButton);

//     // Submit topic container (inline layout)
//     ui::Component submitTopicContainer = ComponentGenerator::createContainer(
//         "submitTopicContainer",
//         "flex gap-2 w-full max-w-md",
//         "",
//         "",
//         "",
//         "",
//         "",
//         ""
//     );

//     // Input field
//     ui::Component topicInput = ComponentGenerator::createInput(
//         "topicInput",
//         "Enter a debate topic...",
//         "topic",
//         "bg-gray-800",
//         "text-white",
//         "border-gray-600",
//         "p-3",
//         "rounded",
//         "flex-1"
//     );
//     ComponentGenerator::addChild(&submitTopicContainer, topicInput);

//     // Submit button
//     ui::Component submitButton = ComponentGenerator::createButton(
//         "submitButton",
//         "Submit",
//         "submit",
//         "bg-green-600",
//         "hover:bg-green-500",
//         "text-white",
//         "px-6 py-3",
//         "rounded-lg",
//         "transition"
//     );
//     ComponentGenerator::addChild(&submitTopicContainer, submitButton);
//     ComponentGenerator::addChild(&main, submitTopicContainer);

//     // Join debate by ID container (inline layout)
//     ui::Component joinDebateContainer = ComponentGenerator::createContainer(
//         "joinDebateContainer",
//         "flex gap-2 w-full max-w-md",
//         "",
//         "",
//         "",
//         "",
//         "",
//         ""
//     );

//     // Join debate input field
//     ui::Component joinDebateInput = ComponentGenerator::createInput(
//         "joinDebateInput",
//         "Enter debate ID...",
//         "debateId",
//         "bg-gray-800",
//         "text-white",
//         "border-gray-600",
//         "p-3",
//         "rounded",
//         "flex-1"
//     );
//     ComponentGenerator::addChild(&joinDebateContainer, joinDebateInput);

//     // Join button
//     ui::Component joinDebateButton = ComponentGenerator::createButton(
//         "joinDebateButton",
//         "Join",
//         "joinDebate",
//         "bg-blue-600",
//         "hover:bg-blue-500",
//         "text-white",
//         "px-6 py-3",
//         "rounded-lg",
//         "transition"
//     );
//     ComponentGenerator::addChild(&joinDebateContainer, joinDebateButton);
//     ComponentGenerator::addChild(&main, joinDebateContainer);

//     // Topics card container
//     ui::Component topicsCard = ComponentGenerator::createContainer(
//         "topicsCard",
//         "",
//         "bg-gray-800",
//         "p-4",
//         "",
//         "",
//         "rounded-lg",
//         "w-full max-w-md shadow"
//     );

//     // Topics header
//     ui::Component topicsHeader = ComponentGenerator::createText(
//         "topicsHeader",
//         "Your Debates",
//         "text-xl",
//         "text-white",
//         "font-semibold",
//         "mb-3"
//     );
//     ComponentGenerator::addChild(&topicsCard, topicsHeader);

//     // Topics list container (empty, will be populated dynamically)
//     ui::Component topicsList = ComponentGenerator::createContainer(
//         "topicsList",
//         "flex flex-col gap-2",
//         "",
//         "",
//         "",
//         "",
//         "",
//         ""
//     );
    
//     // Populate topics list with user's debates
//     for (const auto& topic : usersDebates.topics()) {
//         ui::Component debateItemContainer = ComponentGenerator::createContainer(
//             "debateItemContainer_" + std::to_string(topic.id()),
//             "flex gap-2",
//             "",
//             "",
//             "",
//             "",
//             "",
//             ""
//         );

//         ui::Component debateItemWrapper = ComponentGenerator::createContainer(
//             "debateItemWrapper_" + std::to_string(topic.id()),
//             "flex flex-col flex-1",
//             "bg-gray-700",
//             "p-4",
//             "",
//             "border border-gray-600",
//             "rounded",
//             "transition hover:bg-gray-600"
//         );

//         ui::Component debateItem = ComponentGenerator::createButton(
//             "enterDebateTopicButton_" + std::to_string(topic.id()),
//             topic.topic(),
//             "selectDebate",
//             "",
//             "",
//             "text-white",
//             "",
//             "",
//             "text-left"
//         );
//         ComponentGenerator::addAttribute(&debateItem, "data-debate-id", std::to_string(topic.id()));
//         ComponentGenerator::addChild(&debateItemWrapper, debateItem);

//         if (topic.is_challenge()) {
//             ui::Component challengeInfo = ComponentGenerator::createText(
//                 "challengeInfo_" + std::to_string(topic.id()),
//                 "This is a challenge to claim: " + topic.claim_its_challenging(),
//                 "text-xs",
//                 "text-yellow-400",
//                 "italic",
//                 "px-2 pt-1"
//             );
//             ComponentGenerator::addChild(&debateItemWrapper, challengeInfo);
//         }

//         ComponentGenerator::addChild(&debateItemContainer, debateItemWrapper);

//         if (!topic.is_challenge()) {
//             ui::Component deleteDebateButton = ComponentGenerator::createButton(
//                 "deleteDebateButton_" + std::to_string(topic.id()),
//                 "Delete",
//                 "",
//                 "bg-red-600",
//                 "hover:bg-red-700",
//                 "text-white",
//                 "px-4 py-2",
//                 "rounded",
//                 "transition border border-red-500"
//             );
//             // ComponentGenerator::addChild(&debateItemContainer, deleteDebateButton);
//         }

//         ComponentGenerator::addChild(&topicsList, debateItemContainer);
//     }
    
//     ComponentGenerator::addChild(&topicsCard, topicsList);

//     // Add topics card to main
//     ComponentGenerator::addChild(&main, topicsCard);

//     // Add main container to page
//     ui::Component* pageMain = page.add_components();
//     pageMain->CopyFrom(main);



//     return page;
// }

ui::Component HomePageGenerator::GenerateHomePageMainLayout() {
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

ui::Component HomePageGenerator::FillDebateTopics(user::User user, ui::Component mainLayout) {
    user_engagement::DebateList usersDebates = user.engagement().home_info().available_debates();
    int currentUserId = user.user_id();

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
    for (const auto& topic : usersDebates.topics()) {
        ui::Component debateItemContainer = ComponentGenerator::createContainer(
            "debateItemContainer_" + std::to_string(topic.id()),
            "flex gap-2",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        ui::Component debateItemWrapper = ComponentGenerator::createContainer(
            "debateItemWrapper_" + std::to_string(topic.id()),
            "flex flex-col flex-1",
            "bg-gray-700",
            "p-4",
            "",
            "border border-gray-600",
            "rounded",
            "transition hover:bg-gray-600"
        );

        ui::Component debateItem = ComponentGenerator::createButton(
            "enterDebateTopicButton_" + std::to_string(topic.id()),
            topic.topic(),
            "selectDebate",
            "",
            "",
            "text-white",
            "",
            "",
            "text-left"
        );
        ComponentGenerator::addAttribute(&debateItem, "data-debate-id", std::to_string(topic.id()));
        ComponentGenerator::addChild(&debateItemWrapper, debateItem);

        if (topic.is_challenge()) {
            ui::Component challengeInfo = ComponentGenerator::createText(
                "challengeInfo_" + std::to_string(topic.id()),
                "This is a challenge to claim: " + topic.claim_its_challenging(),
                "text-xs",
                "text-yellow-400",
                "italic",
                "px-2 pt-1"
            );
            ComponentGenerator::addChild(&debateItemWrapper, challengeInfo);
        }

        ComponentGenerator::addChild(&debateItemContainer, debateItemWrapper);

        // Show delete button if user owns the debate and it's not a challenge
        // Show leave button if user doesn't own the debate
        if (!topic.is_challenge()) {
            bool userOwnsDebate = (currentUserId == topic.creator_id());
            
            if (userOwnsDebate) {
                ui::Component deleteDebateButton = ComponentGenerator::createButton(
                    "deleteDebateButton_" + std::to_string(topic.id()),
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
                    "leaveDebateButton_" + std::to_string(topic.id()),
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