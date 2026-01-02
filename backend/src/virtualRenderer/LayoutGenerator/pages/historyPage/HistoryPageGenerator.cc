#include "HistoryPageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"

ui::Page HistoryPageGenerator::GenerateHistoryPage(user::User user) {
    user_engagement::DebateList usersDebates = user.engagement().home_info().available_debates();
    ui::Page page;
    page.set_page_id("history");
    page.set_title("History of Claim");

    // Main container
    ui::Component main = GenerateHistoryPageMainLayout();

    main = FillHistoryItems(user, main);
    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(main);

    return page;
}

ui::Component HistoryPageGenerator::GenerateHistoryPageMainLayout() {
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

    // Back button
    ui::Component backButton = ComponentGenerator::createButton(
        "backButton",
        "← Back",
        "",
        "bg-gray-700",
        "hover:bg-gray-600",
        "text-white",
        "px-6 py-2",
        "rounded",
        "transition-colors self-start mb-4"
    );
    ComponentGenerator::addChild(&main, backButton);

    // Title
    ui::Component titleText = ComponentGenerator::createText(
        "historyTitle",
        "History of Claim: ",
        "text-3xl",
        "text-white",
        "font-bold",
        "mb-6"
    );
    ComponentGenerator::addChild(&main, titleText);

    // Claim sentence container (to be filled)
    ui::Component claimSentenceContainer = ComponentGenerator::createContainer(
        "claimSentenceContainer",
        "",
        "",
        "",
        "",
        "",
        "",
        ""
    );
    ComponentGenerator::addChild(&main, claimSentenceContainer);

    // History items list container (empty for now)
    ui::Component historyListContainer = ComponentGenerator::createContainer(
        "historyListContainer",
        "flex flex-col gap-4",
        "",
        "",
        "w-full max-w-4xl",
        "",
        "",
        ""
    );
    ComponentGenerator::addChild(&main, historyListContainer);

    return main;
}

ui::Component HistoryPageGenerator::FillHistoryItems(user::User user, ui::Component mainLayout) {
    
    return mainLayout;
}