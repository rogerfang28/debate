#include "DebateClaimPageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"

ui::Page DebateClaimPageGenerator::GenerateDebatePage(const std::string& claimTitle, const std::string& claimDescription) {
    ui::Page page;
    page.set_page_id("debate");
    page.set_title("Debate View");

    // Main layout container
    ui::Component mainLayout = ComponentGenerator::createContainer(
        "mainLayout",
        "",
        "bg-gray-900",
        "p-4",
        "",
        "",
        "",
        "min-h-screen"
    );

    // Top section container
    ui::Component topSection = ComponentGenerator::createContainer(
        "topSection",
        "flex justify-between items-start",
        "",
        "",
        "mb-8",
        "",
        "",
        ""
    );

    // Left top section - navigation buttons
    ui::Component leftTopSection = ComponentGenerator::createContainer(
        "leftTopSection",
        "flex flex-col gap-2",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    ui::Component goToParentButton = ComponentGenerator::createButton(
        "goToParentButton",
        "Go to Parent",
        "",
        "bg-gray-600",
        "hover:bg-gray-700",
        "text-white",
        "px-4 py-2",
        "rounded",
        "transition-colors text-sm"
    );
    ComponentGenerator::addChild(&leftTopSection, goToParentButton);

    ui::Component goHomeButton = ComponentGenerator::createButton(
        "goHomeButton",
        "Home",
        "",
        "bg-blue-600",
        "hover:bg-blue-700",
        "text-white",
        "px-4 py-2",
        "rounded",
        "transition-colors text-sm"
    );
    ComponentGenerator::addChild(&leftTopSection, goHomeButton);
    ComponentGenerator::addChild(&topSection, leftTopSection);

    // Focus section (center)
    ui::Component focusSection = ComponentGenerator::createContainer(
        "focusSection",
        "flex-1",
        "",
        "",
        "mx-8",
        "",
        "",
        ""
    );

    ui::Component focusingOnClaim = ComponentGenerator::createContainer(
        "focusingOnClaim",
        "text-center",
        "bg-gray-700",
        "py-4 px-6",
        "",
        "border",
        "rounded",
        ""
    );

    ui::Component focusLabel = ComponentGenerator::createText(
        "focusLabel",
        "Focusing on claim:",
        "text-sm",
        "text-white",
        "",
        "mb-2"
    );
    ComponentGenerator::addChild(&focusingOnClaim, focusLabel);

    ui::Component currentClaimTitle = ComponentGenerator::createText(
        "currentClaimTitle",
        claimTitle,
        "text-lg",
        "text-white",
        "font-semibold",
        ""
    );
    ComponentGenerator::addChild(&focusingOnClaim, currentClaimTitle);
    ComponentGenerator::addChild(&focusSection, focusingOnClaim);
    ComponentGenerator::addChild(&topSection, focusSection);

    // Right top section - minimap
    ui::Component rightTopSection = ComponentGenerator::createContainer(
        "rightTopSection",
        "",
        "",
        "",
        "",
        "",
        "",
        "w-64"
    );

    ui::Component minimap = ComponentGenerator::createContainer(
        "minimap",
        "",
        "bg-gray-800",
        "p-2",
        "",
        "border-2 border-gray-300",
        "rounded",
        "w-64 h-48"
    );

    ui::Component minimapTitle = ComponentGenerator::createText(
        "minimapTitle",
        "minimap",
        "text-xs",
        "text-gray-500",
        "",
        "mb-2"
    );
    ComponentGenerator::addChild(&minimap, minimapTitle);

    ui::Component minimapContent = ComponentGenerator::createContainer(
        "minimapContent",
        "grid grid-cols-4 gap-1 h-full",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    // Add minimap nodes
    for (int i = 1; i <= 8; i++) {
        std::string nodeId = "minimapNode" + std::to_string(i);
        std::string bgColor = (i == 1) ? "bg-yellow-200" : "bg-green-400";
        ui::Component minimapNode = ComponentGenerator::createContainer(
            nodeId,
            "",
            bgColor,
            "",
            "",
            "border",
            "rounded",
            ""
        );
        ComponentGenerator::addChild(&minimapContent, minimapNode);
    }

    ComponentGenerator::addChild(&minimap, minimapContent);
    ComponentGenerator::addChild(&rightTopSection, minimap);
    ComponentGenerator::addChild(&topSection, rightTopSection);
    ComponentGenerator::addChild(&mainLayout, topSection);

    // Content area
    ui::Component contentArea = ComponentGenerator::createContainer(
        "contentArea",
        "flex gap-8",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    // Left content - guidance box
    ui::Component leftContent = ComponentGenerator::createContainer(
        "leftContent",
        "",
        "",
        "",
        "",
        "",
        "",
        "w-96"
    );

    ui::Component guidanceBox = ComponentGenerator::createContainer(
        "guidanceBox",
        "",
        "bg-gray-800",
        "p-6",
        "",
        "border-2 border-gray-700",
        "rounded",
        "min-h-64"
    );

    ui::Component guidanceText = ComponentGenerator::createText(
        "guidanceText",
        "Guide: ",
        "",
        "text-white",
        "",
        ""
    );
    ComponentGenerator::addChild(&guidanceBox, guidanceText);
    ComponentGenerator::addChild(&leftContent, guidanceBox);
    ComponentGenerator::addChild(&contentArea, leftContent);

    // Center content
    ui::Component centerContent = ComponentGenerator::createContainer(
        "centerContent",
        "flex-1",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    // Description box
    ui::Component descriptionBox = ComponentGenerator::createContainer(
        "descriptionBox",
        "",
        "bg-gray-800",
        "p-6",
        "mb-8",
        "border-2 border-gray-700",
        "rounded",
        ""
    );

    ui::Component descriptionTitle = ComponentGenerator::createText(
        "descriptionTitle",
        "Claim Description:",
        "text-lg",
        "text-white",
        "font-semibold",
        "mb-4"
    );
    ComponentGenerator::addChild(&descriptionBox, descriptionTitle);

    ui::Component descriptionContent = ComponentGenerator::createText(
        "descriptionContent",
        claimDescription,
        "",
        "text-white",
        "",
        "leading-relaxed mb-6"
    );
    ComponentGenerator::addChild(&descriptionBox, descriptionContent);

    // Description actions
    ui::Component descriptionActions = ComponentGenerator::createContainer(
        "descriptionActions",
        "flex gap-3",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    ui::Component editDescriptionButton = ComponentGenerator::createButton(
        "editDescriptionButton",
        "Edit Description",
        "",
        "bg-yellow-600",
        "hover:bg-yellow-700",
        "text-white",
        "px-4 py-2",
        "rounded",
        "transition-colors text-sm"
    );
    ComponentGenerator::addChild(&descriptionActions, editDescriptionButton);

    ui::Component addCounterArgument = ComponentGenerator::createButton(
        "addCounterArgument",
        "Add Counter-Argument",
        "",
        "bg-red-600",
        "hover:bg-red-700",
        "text-white",
        "px-4 py-2",
        "rounded",
        "transition-colors text-sm"
    );
    ComponentGenerator::addChild(&descriptionActions, addCounterArgument);

    ui::Component addSupportingEvidence = ComponentGenerator::createButton(
        "addSupportingEvidence",
        "Add Supporting Evidence",
        "",
        "bg-green-600",
        "hover:bg-green-700",
        "text-white",
        "px-4 py-2",
        "rounded",
        "transition-colors text-sm"
    );
    ComponentGenerator::addChild(&descriptionActions, addSupportingEvidence);
    ComponentGenerator::addChild(&descriptionBox, descriptionActions);
    ComponentGenerator::addChild(&centerContent, descriptionBox);

    // Child arguments section
    ui::Component childArgumentsSection = ComponentGenerator::createContainer(
        "childArgumentsSection",
        "",
        "bg-gray-800",
        "p-6",
        "",
        "border-2 border-gray-700",
        "rounded",
        ""
    );

    ui::Component childArgumentsTitle = ComponentGenerator::createText(
        "childArgumentsTitle",
        "Child Arguments:",
        "text-lg",
        "text-white",
        "font-semibold",
        "mb-6"
    );
    ComponentGenerator::addChild(&childArgumentsSection, childArgumentsTitle);

    ui::Component childArgumentsGrid = ComponentGenerator::createContainer(
        "childArgumentsGrid",
        "grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    // Example child nodes (3 nodes as in the pbtxt)
    std::vector<std::pair<std::string, std::string>> childNodes = {
        {"some child node", "button to go to this node"},
        {"another", "button"},
        {"another", "nutton"}
    };

    for (size_t i = 0; i < childNodes.size(); i++) {
        std::string nodeId = "childNode" + std::to_string(i + 1);
        ui::Component childNode = ComponentGenerator::createContainer(
            nodeId,
            "",
            "bg-gray-700",
            "p-4",
            "",
            "border border-gray-700",
            "rounded",
            ""
        );

        ui::Component childNodeTitle = ComponentGenerator::createText(
            nodeId + "Title",
            childNodes[i].first,
            "",
            "text-white",
            "",
            "mb-3"
        );
        ComponentGenerator::addChild(&childNode, childNodeTitle);

        ui::Component childNodeButton = ComponentGenerator::createButton(
            nodeId + "Button",
            childNodes[i].second,
            "",
            "bg-blue-600",
            "hover:bg-blue-700",
            "text-white",
            "px-4 py-2",
            "rounded",
            "w-full transition-colors text-sm"
        );
        ComponentGenerator::addChild(&childNode, childNodeButton);
        ComponentGenerator::addChild(&childArgumentsGrid, childNode);
    }

    ComponentGenerator::addChild(&childArgumentsSection, childArgumentsGrid);
    ComponentGenerator::addChild(&centerContent, childArgumentsSection);
    ComponentGenerator::addChild(&contentArea, centerContent);

    // Right content - report section
    ui::Component rightContent = ComponentGenerator::createContainer(
        "rightContent",
        "",
        "",
        "",
        "mt-16",
        "",
        "",
        "w-80"
    );

    ui::Component reportSection = ComponentGenerator::createContainer(
        "reportSection",
        "",
        "bg-gray-800",
        "p-6",
        "",
        "border-2 border-gray-700",
        "rounded",
        ""
    );

    ui::Component reportTitle = ComponentGenerator::createText(
        "reportTitle",
        "Think this is wrong? File a claim report.",
        "text-sm",
        "text-white",
        "",
        "mb-6"
    );
    ComponentGenerator::addChild(&reportSection, reportTitle);

    ui::Component reportButton = ComponentGenerator::createButton(
        "reportButton",
        "Report Claim",
        "",
        "bg-red-600",
        "hover:bg-red-700",
        "text-white",
        "px-4 py-2",
        "rounded",
        "w-full transition-colors"
    );
    ComponentGenerator::addChild(&reportSection, reportButton);
    ComponentGenerator::addChild(&rightContent, reportSection);
    ComponentGenerator::addChild(&contentArea, rightContent);
    ComponentGenerator::addChild(&mainLayout, contentArea);

    // Add main layout to page
    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(mainLayout);

    return page;
}


