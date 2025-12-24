#include "HomePageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"

ui::Page HomePageGenerator::GenerateHomePage(user_engagement::DebateList usersDebates) {
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
        "Debate Topics",
        "text-3xl",
        "text-blue-400",
        "font-bold",
        ""
    );
    ComponentGenerator::addChild(&main, title);

    // Description
    ui::Component description = ComponentGenerator::createText(
        "description",
        "Type a topic and submit. Existing topics appear below.",
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
    ComponentGenerator::addChild(&main, clearButton);

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
        "w-full max-w-md"
    );
    ComponentGenerator::addChild(&main, topicInput);

    // Submit button
    ui::Component submitButton = ComponentGenerator::createButton(
        "submitButton",
        "Submit Topic",
        "submit",
        "bg-green-600",
        "hover:bg-green-500",
        "text-white",
        "px-6 py-3",
        "rounded-lg",
        "transition"
    );
    ComponentGenerator::addChild(&main, submitButton);

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
        "Submitted Topics",
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
        ui::Component debateItem = ComponentGenerator::createButton(
            "enterDebateTopicButton_" + topic.id(),
            topic.topic(),
            "selectDebate",
            "bg-gray-700",
            "hover:bg-gray-600",
            "text-white",
            "px-4 py-2",
            "rounded",
            "w-full text-left transition border border-gray-600"
        );
        ComponentGenerator::addAttribute(&debateItem, "data-debate-id", topic.id());
        ComponentGenerator::addChild(&topicsList, debateItem);
    }
    
    ComponentGenerator::addChild(&topicsCard, topicsList);

    // Add topics card to main
    ComponentGenerator::addChild(&main, topicsCard);

    // Add main container to page
    ui::Component* pageMain = page.add_components();
    pageMain->CopyFrom(main);



    return page;
}