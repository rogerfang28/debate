#include "HomePageEventParser.h"
#include <iostream>

debate_event::DebateEvent HomePageEventParser::ParseHomePageEvent(
    const std::string& componentId,
    const std::string& eventType,
    // const std::string& user,
    const client_message::ClientMessage& message
) {
    debate_event::DebateEvent event;
    
    if (componentId == "submitButton" && eventType == "onClick") {
        event.set_type(debate_event::CREATE_DEBATE);
        auto* create = event.mutable_create_debate();
        
        // Extract topicInput value from page_data
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "topicInput") {
                create->set_debate_topic(comp.value());
                std::cout << "  CREATE_DEBATE: topic = " << comp.value() << "\n";
                break;
            }
        }
    } else if (componentId == "clearButton" && eventType == "onClick") {
        event.set_type(debate_event::CLEAR_DEBATES);
        auto* clear = event.mutable_clear_debates();
        // std::cout << "  CLEAR_DEBATES for user: " << user << "\n";
    } else if (componentId.find("enterDebateTopicButton_") == 0 && eventType == "onClick") {
        // Enter a debate
        event.set_type(debate_event::ENTER_DEBATE);
        auto* enter = event.mutable_enter_debate();
        std::string debateID = componentId.substr(23); // Extract ID after "enterDebateTopicButton_"
        enter->set_debate_id(debateID);
        std::cout << "  ENTER_DEBATE: debate_id = " << debateID << "\n";
    } else {
        std::cerr << "Unknown component/event combination on home page: " 
                  << componentId << "/" << eventType << "\n";
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    return event;
}