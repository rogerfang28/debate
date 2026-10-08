#include "HomePageEventParser.h"
#include <cstdlib>
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
    // * Build a whole debate from a passage, with the AI
    else if (componentId == "generateDebateButton" && eventType == "onClick") {
        event.set_type(debate_event::GENERATE_DEBATE);
        auto* generate = event.mutable_generate_debate();
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "generateTopicInput") {
                generate->set_topic(comp.value());
            } else if (comp.id() == "generatePassageInput") {
                generate->set_passage(comp.value());
            }
        }
        Log::debug("  GENERATE_DEBATE: topic = " + generate->topic() + ", passage = " +
                   std::to_string(generate->passage().size()) + " chars");
    }
    // * Guided walkthrough. The id carries the script name after the underscore
    // so the home page can offer several without the parser knowing any of them.
    else if (componentId.rfind("startTutorialButton", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::START_TUTORIAL);
        const std::string prefix = "startTutorialButton_";
        if (componentId.rfind(prefix, 0) == 0) {
            event.mutable_start_tutorial()->set_script(componentId.substr(prefix.size()));
        }
        Log::debug("  START_TUTORIAL: script = '" + event.start_tutorial().script() + "'");
    }
    else if (componentId == "cancelAiWorkButton" && eventType == "onClick") {
        event.set_type(debate_event::CANCEL_AI_WORK);
    }
    else if (componentId == "dismissAiNoticeButton" && eventType == "onClick") {
        event.set_type(debate_event::DISMISS_AI_NOTICE);
    }
    // * Clear all debates
    else if (componentId == "clearButton" && eventType == "onClick") {
        event.set_type(debate_event::CLEAR_DEBATES);
    } 

    // * A card on the shelf: enter that debate on its Brief tab
    else if (componentId.rfind("openDebateCard_", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::OPEN_DEBATE_BRIEF);
        const std::string debateID = componentId.substr(std::string("openDebateCard_").size());
        event.mutable_enter_debate()->set_debate_id(std::atoi(debateID.c_str()));
        Log::debug("  OPEN_DEBATE_BRIEF: debate_id = " + debateID);
    }

    // * A visitor's "Start a debate": an empty create, which the moderator
    // turns into the sign-in modal and replays (as a no-op) once they have
    // signed in, leaving them at the create form.
    else if (componentId == "startDebateButton" && eventType == "onClick") {
        event.set_type(debate_event::CREATE_DEBATE);
        event.mutable_create_debate();
    }

    // * Delete a debate with id
    else if (componentId.find("deleteDebateButton_") == 0 && eventType == "onClick") {
        event.set_type(debate_event::DELETE_DEBATE);
        auto* deleteDebate = event.mutable_delete_debate();
        std::string debateID = componentId.substr(19);
        deleteDebate->set_debate_id(std::stoi(debateID));
        Log::debug("  DELETE_DEBATE: debate_id = " + debateID);
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