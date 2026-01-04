#include "ErrorPageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"

ui::Page ErrorPageGenerator::GenerateErrorPage() {
    ui::Page page;
    page.set_page_id("error");
    page.set_title("Error - Page Not Found");

    // Main container
    ui::Component mainLayout = GenerateErrorPageMainLayout();
    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(mainLayout);

    return page;
}

ui::Component ErrorPageGenerator::GenerateErrorPageMainLayout() {
    // Main container
    ui::Component main = ComponentGenerator::createContainer(
        "main",
        "min-h-screen flex flex-col items-center justify-center gap-6",
        "bg-gray-900",
        "p-8",
        "",
        "",
        "",
        "text-white"
    );

    
    // Help text
    ui::Component helpText = ComponentGenerator::createText(
        "helpText",
        "If the home button doesn't work, try clearing cookies on the website.",
        "text-sm",
        "text-gray-500",
        "mt-2",
        "text-center"
    );
    ComponentGenerator::addChild(&main, helpText);

    // Title
    ui::Component title = ComponentGenerator::createText(
        "title",
        "404 - Page Not Found",
        "text-3xl",
        "text-red-400",
        "font-bold",
        ""
    );
    ComponentGenerator::addChild(&main, title);

    // Description
    ui::Component description = ComponentGenerator::createText(
        "description",
        "The page you are looking for does not exist. Please return to the home page.",
        "",
        "text-gray-300",
        "",
        "text-center"
    );
    ComponentGenerator::addChild(&main, description);

    // Home button
    ui::Component homeButton = ComponentGenerator::createButton(
        "goHomeButton",
        "Go to Home Page",
        "goHome",
        "bg-blue-600",
        "hover:bg-blue-500",
        "text-white",
        "px-4 py-2",
        "rounded-lg",
        "transition"
    );
    ComponentGenerator::addChild(&main, homeButton);

    return main;
}