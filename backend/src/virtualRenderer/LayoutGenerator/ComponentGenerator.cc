#include "ComponentGenerator.h"

ui::Component ComponentGenerator::createText(
    const std::string& id,
    const std::string& text,
    const std::string& textSize,
    const std::string& textColor,
    const std::string& fontWeight,
    const std::string& additionalClasses
) {
    ui::Component component;
    component.set_id(id);
    component.set_type(ui::ComponentType::TEXT);
    component.set_text(text);
    
    std::vector<std::string> classes;
    if (!textSize.empty()) classes.push_back(textSize);
    if (!textColor.empty()) classes.push_back(textColor);
    if (!fontWeight.empty()) classes.push_back(fontWeight);
    if (!additionalClasses.empty()) classes.push_back(additionalClasses);
    
    std::string customClass = buildCustomClass(classes);
    if (!customClass.empty()) {
        component.mutable_style()->set_custom_class(customClass);
    }
    
    return component;
}

ui::Component ComponentGenerator::createButton(
    const std::string& id,
    const std::string& text,
    const std::string& name,
    const std::string& bgColor,
    const std::string& hoverColor,
    const std::string& textColor,
    const std::string& padding,
    const std::string& rounded,
    const std::string& additionalClasses
) {
    ui::Component component;
    component.set_id(id);
    component.set_type(ui::ComponentType::BUTTON);
    component.set_text(text);
    
    if (!name.empty()) {
        component.set_name(name);
    }
    
    std::vector<std::string> classes;
    if (!padding.empty()) classes.push_back(padding);
    if (!bgColor.empty()) classes.push_back(bgColor);
    if (!hoverColor.empty()) classes.push_back(hoverColor);
    if (!textColor.empty()) classes.push_back(textColor);
    if (!rounded.empty()) classes.push_back(rounded);
    classes.push_back("transition-colors");
    if (!additionalClasses.empty()) classes.push_back(additionalClasses);
    
    std::string customClass = buildCustomClass(classes);
    if (!customClass.empty()) {
        component.mutable_style()->set_custom_class(customClass);
    }
    
    return component;
}

ui::Component ComponentGenerator::createInput(
    const std::string& id,
    const std::string& placeholder,
    const std::string& name,
    const std::string& bgColor,
    const std::string& textColor,
    const std::string& borderColor,
    const std::string& padding,
    const std::string& rounded,
    const std::string& additionalClasses,
    const std::string& defaultValue
    // optional default value parameter could be added here
) {
    ui::Component component;
    component.set_id(id);
    component.set_type(ui::ComponentType::INPUT);
    
    if (!name.empty()) {
        component.set_name(name);
    }
    
    if (!placeholder.empty()) {
        addAttribute(&component, "placeholder", placeholder);
    }
    
    if (!defaultValue.empty()) {
        addAttribute(&component, "defaultValue", defaultValue);
    }
    
    std::vector<std::string> classes;
    if (!padding.empty()) classes.push_back(padding);
    if (!bgColor.empty()) classes.push_back(bgColor);
    if (!textColor.empty()) classes.push_back(textColor);
    classes.push_back("border");
    if (!borderColor.empty()) classes.push_back(borderColor);
    if (!rounded.empty()) classes.push_back(rounded);
    if (!additionalClasses.empty()) classes.push_back(additionalClasses);
    
    std::string customClass = buildCustomClass(classes);
    if (!customClass.empty()) {
        component.mutable_style()->set_custom_class(customClass);
    }
    
    return component;
}

ui::Component ComponentGenerator::createContainer(
    const std::string& id,
    const std::string& layout,
    const std::string& bgColor,
    const std::string& padding,
    const std::string& margin,
    const std::string& border,
    const std::string& rounded,
    const std::string& additionalClasses
) {
    ui::Component component;
    component.set_id(id);
    component.set_type(ui::ComponentType::CONTAINER);
    
    std::vector<std::string> classes;
    if (!layout.empty()) classes.push_back(layout);
    if (!bgColor.empty()) classes.push_back(bgColor);
    if (!padding.empty()) classes.push_back(padding);
    if (!margin.empty()) classes.push_back(margin);
    if (!border.empty()) classes.push_back(border);
    if (!rounded.empty()) classes.push_back(rounded);
    if (!additionalClasses.empty()) classes.push_back(additionalClasses);
    
    std::string customClass = buildCustomClass(classes);
    if (!customClass.empty()) {
        component.mutable_style()->set_custom_class(customClass);
    }
    
    return component;
}

void ComponentGenerator::addChild(ui::Component* parent, const ui::Component& child) {
    if (parent) {
        ui::Component* addedChild = parent->add_children();
        addedChild->CopyFrom(child);
    }
}

std::string ComponentGenerator::buildCustomClass(const std::vector<std::string>& classes) {
    std::string result;
    for (size_t i = 0; i < classes.size(); ++i) {
        if (!classes[i].empty()) {
            if (!result.empty()) result += " ";
            result += classes[i];
        }
    }
    return result;
}

void ComponentGenerator::addAttribute(ui::Component* component, const std::string& key, const std::string& value) {
    if (component && !key.empty()) {
        (*component->mutable_attributes())[key] = value;
    }
}
