#include "HomePageEventParser.h"
#include <iostream>
#include "../../../utils/Log.h"

debate_event::DebateEvent HomePageEventParser::ParseHomePageEvent(
    const std::string& componentId,
    const std::string& eventType,
    const client_message::ClientMessage& message
) {
    debate_event::DebateEvent event;
    
    // * Submit a debate topic, creates a new debate
    if (componentId == "submitButton" && eventType == "onClick") {
        event.set_type(debate_event::CREATE_DEBATE);
        auto* create = event.mutable_create_debate();
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "topicInput") {
                create->set_debate_topic(comp.value());
                Log::debug("  CREATE_DEBATE: topic = " + comp.value());
                break;
            }
        }
    } 
    // * Clear all debates
    else if (componentId == "clearButton" && eventType == "onClick") {
        event.set_type(debate_event::CLEAR_DEBATES);
        auto* clear = event.mutable_clear_debates();
    } 

    // * Enter a debate, moves user to debate page
    else if (componentId.find("enterDebateTopicButton_") == 0 && eventType == "onClick") {
        event.set_type(debate_event::ENTER_DEBATE);
        auto* enter = event.mutable_enter_debate();
        std::string debateID = componentId.substr(23);
        enter->set_debate_id(std::stoi(debateID));
        Log::debug("  ENTER_DEBATE: debate_id = " + debateID);
    } 
    
    // * Delete a debate with id
    else if (componentId.find("deleteDebateButton_") == 0 && eventType == "onClick") {
        event.set_type(debate_event::DELETE_DEBATE);
        auto* deleteDebate = event.mutable_delete_debate();
        std::string debateID = componentId.substr(19);
        deleteDebate->set_debate_id(std::stoi(debateID));
        Log::debug("  DELETE_DEBATE: debate_id = " + debateID);
    }

    // * Join a debate with id
    else if (componentId.find("joinDebateButton") == 0 && eventType == "onClick") {
        event.set_type(debate_event::JOIN_DEBATE);
        auto* joinDebate = event.mutable_join_debate();
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "joinDebateInput") {
                joinDebate->set_debate_id(std::stoi(comp.value()));
                Log::debug("  JOIN_DEBATE: debate_id = " + comp.value());
                break;
            }
        }
    }

    // ! Logout event, which won't happen because virtual renderer handleAuthEvents is already doing it
    else if (componentId == "logoutButton"){
        Log::debug("  LOGOUT event");
        event.set_type(debate_event::LOGOUT); // backend can't handle logout since it's based on the cookie from the request
    }
    
    else {
        Log::error("Unknown component/event combination on home page: " 
                  + componentId + "/" + eventType);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    return event;
}