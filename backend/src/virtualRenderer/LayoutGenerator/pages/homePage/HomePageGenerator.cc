#include "HomePageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"

ui::Page HomePageGenerator::GenerateHomePage(user_engagement::NoneInfo info) {
    user_engagement::DebateList usersDebates = info.available_debates();
    ui::Page page;
    page.set_page_id("home");
    page.set_title("Debate Topic Entry");

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

    // Clear button
    ui::Component clearButton = ComponentGenerator::createButton(
        "clearButton",
        "Clear All Topics",
        "clear",
        "bg-red-600",
        "hover:bg-red-500",
        "text-white",
        "px-4 py-2",
        "rounded-lg",
        "transition"
    );
    // ComponentGenerator::addChild(&main, clearButton);

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
        "px-6 py-3",
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
        "px-6 py-3",
        "rounded-lg",
        "transition"
    );
    ComponentGenerator::addChild(&joinDebateContainer, joinDebateButton);
    ComponentGenerator::addChild(&main, joinDebateContainer);

    // Topics card container
    ui::Component topicsCard = ComponentGenerator::createContainer(
        "topicsCard",
        "",
        "bg-gray-800",
        "p-4",
        "",
        "",
        "rounded-lg",
        "w-full max-w-md shadow"
    );

    // Topics header
    ui::Component topicsHeader = ComponentGenerator::createText(
        "topicsHeader",
        "Your Debates",
        "text-xl",
        "text-white",
        "font-semibold",
        "mb-3"
    );
    ComponentGenerator::addChild(&topicsCard, topicsHeader);

    // Topics list container (empty, will be populated dynamically)
    ui::Component topicsList = ComponentGenerator::createContainer(
        "topicsList",
        "flex flex-col gap-2",
        "",
        "",
        "",
        "",
        "",
        ""
    );
    
    // Populate topics list with user's debates
    for (const auto& topic : usersDebates.topics()) {
        ui::Component debateItemContainer = ComponentGenerator::createContainer(
            "debateItemContainer_" + topic.id(),
            "flex gap-2",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        ui::Component debateItem = ComponentGenerator::createButton(
            "enterDebateTopicButton_" + topic.id(),
            topic.topic(),
            "selectDebate",
            "bg-gray-700",
            "hover:bg-gray-600",
            "text-white",
            "px-4 py-2",
            "rounded",
            "flex-1 text-left transition border border-gray-600"
        );
        ComponentGenerator::addAttribute(&debateItem, "data-debate-id", topic.id());
        ComponentGenerator::addChild(&debateItemContainer, debateItem);

        ui::Component deleteDebateButton = ComponentGenerator::createButton(
            "deleteDebateButton_" + topic.id(),
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

        ComponentGenerator::addChild(&topicsList, debateItemContainer);
    }
    
    ComponentGenerator::addChild(&topicsCard, topicsList);

    // Add topics card to main
    ComponentGenerator::addChild(&main, topicsCard);

    // Add main container to page
    ui::Component* pageMain = page.add_components();
    pageMain->CopyFrom(main);



    return page;
}