#ifndef COMPONENT_GENERATOR_H
#define COMPONENT_GENERATOR_H

#include <string>
#include <vector>
#include "../../../../src/gen/cpp/layout.pb.h"

class ComponentGenerator {
public:
    // Create a TEXT component
    static ui::Component createText(
        const std::string& id,
        const std::string& text,
        const std::string& textSize = "text-base",
        const std::string& textColor = "text-white",
        const std::string& fontWeight = "",
        const std::string& additionalClasses = ""
    );

    // Create a BUTTON component
    static ui::Component createButton(
        const std::string& id,
        const std::string& text,
        const std::string& name = "",
        const std::string& bgColor = "bg-blue-600",
        const std::string& hoverColor = "hover:bg-blue-500",
        const std::string& textColor = "text-white",
        const std::string& padding = "px-4 py-2",
        const std::string& rounded = "rounded-lg",
        const std::string& additionalClasses = ""
    );

    // Create an INPUT component
    static ui::Component createInput(
        const std::string& id,
        const std::string& placeholder = "",
        const std::string& name = "",
        const std::string& bgColor = "bg-gray-800",
        const std::string& textColor = "text-white",
        const std::string& borderColor = "border-gray-600",
        const std::string& padding = "p-3",
        const std::string& rounded = "rounded",
        const std::string& additionalClasses = ""
    );

    // Create a CONTAINER component
    static ui::Component createContainer(
        const std::string& id,
        const std::string& layout = "",
        const std::string& bgColor = "",
        const std::string& padding = "",
        const std::string& margin = "",
        const std::string& border = "",
        const std::string& rounded = "",
        const std::string& additionalClasses = ""
    );

    // Helper: Add a child component to a parent
    static void addChild(ui::Component* parent, const ui::Component& child);

    // Helper: Build custom_class string from Tailwind utility classes
    static std::string buildCustomClass(const std::vector<std::string>& classes);

    // Helper: Add an attribute to a component
    static void addAttribute(ui::Component* component, const std::string& key, const std::string& value);
};

#endif // COMPONENT_GENERATOR_H
