#include "ClientMessageParser.h"
#include <string>
#include <iostream>
#include <chrono>
#include <ctime>
#include "../../../../src/gen/cpp/client_message.pb.h"
#include "../../../../src/gen/cpp/debate_event.pb.h"

debate_event::DebateEvent ClientMessageParser::parseMessage(const client_message::ClientMessage& message, const std::string& user) {
    debate_event::DebateEvent event;

    // check to see if there is nothing actually happening, like nothing clicked, it's basically no event
    if (message.component_id().empty() && message.event_type().empty()) {
        std::cout << "No action in ClientMessage, returning empty DebateEvent.\n";
        event.set_type(debate_event::NONE);
        return event;
    }

    // Set common fields
    event.set_id("evt_" + std::to_string(std::time(nullptr)));
    
    // Set timestamp to current time
    auto* timestamp = event.mutable_occurred_at();
    auto now = std::chrono::system_clock::now();
    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    timestamp->set_seconds(seconds);

    const std::string& componentId = message.component_id();
    const std::string& eventType = message.event_type();
    const std::string& pageId = message.page_data().page_id();
    
    std::cout << "\n--- Translating ClientMessage to DebateEvent ---\n";
    std::cout << "  Page ID: " << pageId << "\n";
    std::cout << "  Component ID: " << componentId << "\n";
    std::cout << "  Event Type: " << eventType << "\n";
    
    // Route based on page_id and component_id + event_type
    if (pageId == "home") {
        // ============ HOME PAGE ============
        if (componentId == "submitButton" && eventType == "onClick") {
            // Create debate
            event.set_type(debate_event::CREATE_DEBATE);
            auto* create = event.mutable_create_debate();
            create->set_user_id(user);
            
            // Extract topicInput value from page_data
            for (const auto& comp : message.page_data().components()) {
                if (comp.id() == "topicInput") {
                    create->set_debate_topic(comp.value());
                    std::cout << "  CREATE_DEBATE: topic = " << comp.value() << "\n";
                    break;
                }
            }
        } else if (componentId == "clearButton" && eventType == "onClick") {
            // Clear all debates
            event.set_type(debate_event::CLEAR_DEBATES);
            auto* clear = event.mutable_clear_debates();
            clear->set_user_id(user);
            std::cout << "  CLEAR_DEBATES for user: " << user << "\n";
        } else if (componentId.find("topicButton_") == 0 && eventType == "onClick") {
            // Enter a debate
            event.set_type(debate_event::ENTER_DEBATE);
            auto* enter = event.mutable_enter_debate();
            enter->set_user_id(user);
            std::string debateID = componentId.substr(12); // Extract ID after "topicButton_"
            enter->set_debate_id(debateID);
            std::cout << "  ENTER_DEBATE: debate_id = " << debateID << "\n";
        } else {
            std::cerr << "Unknown component/event combination on home page: " 
                      << componentId << "/" << eventType << "\n";
            event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
        }
    } else {
        // ============ OTHER PAGES ============
        if (componentId == "goHomeButton" && eventType == "onClick") {
            event.set_type(debate_event::GO_HOME);
            auto* goHome = event.mutable_go_home();
            goHome->set_user_id(user);
            std::cout << "  GO_HOME for user: " << user << "\n";
        } else {
            std::cerr << "Unknown page or event: page=" << pageId 
                      << " component=" << componentId << "/" << eventType << "\n";
            event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
        }
    }

    std::cout << "  Event Type: " << event.type() << "\n";
    std::cout << "========================================\n\n";
    
    return event;
}