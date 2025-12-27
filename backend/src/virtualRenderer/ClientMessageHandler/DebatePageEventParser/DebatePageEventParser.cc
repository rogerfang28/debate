#include "DebatePageEventParser.h"
#include <iostream>
#include "../../../utils/Log.h"

debate_event::DebateEvent DebatePageEventParser::ParseDebatePageEvent(
    const std::string& componentId,
    const std::string& eventType,
    const std::string& user,
    const client_message::ClientMessage& message
) {
    debate_event::DebateEvent event;
    
    if (componentId == "goHomeButton" && eventType == "onClick") {
        event.set_type(debate_event::GO_HOME);
        auto* goHome = event.mutable_go_home();
        Log::debug("  GO_HOME for user: " + user);
    } else if (componentId == "goToParentButton" && eventType == "onClick") {
        Log::debug("  GO_TO_PARENT for user: " + user);
        event.set_type(debate_event::GO_TO_PARENT);
    } else if (componentId == "deleteStatementButton" && eventType == "onClick") {
        Log::debug("  DELETE_CURRENT_STATEMENT for user: " + user);
        event.set_type(debate_event::DELETE_CURRENT_STATEMENT);
    } else if (componentId.rfind("deleteChildClaimButton_", 0) == 0 && eventType == "onClick") {
        std::string claimId = componentId.substr(strlen("deleteChildClaimButton_"));
        Log::debug("  DELETE_CHILD_CLAIM for user: " + user + " claim ID: " + claimId);
        event.set_type(debate_event::DELETE_CHILD_CLAIM); // Placeholder, replace with actual DELETE_CHILD_CLAIM type
        event.mutable_delete_child_claim()->set_claim_id(claimId);
    } else if (componentId == "reportButton" && eventType == "onClick") {
        Log::debug("  REPORT_CLAIM for user: " + user);
        event.set_type(debate_event::REPORT_CLAIM);

    // edit description
    } else if (componentId == "editDescriptionButton" && eventType == "onClick") {
        Log::debug("  EDIT_DESCRIPTION for user: " + user);
        event.set_type(debate_event::START_EDIT_CLAIM_DESCRIPTION);
    } else if (componentId == "saveDescriptionButton" && eventType == "onClick") {
        Log::debug("  SUBMIT_EDIT_CLAIM_DESCRIPTION for user: " + user);
        event.set_type(debate_event::SUBMIT_EDIT_CLAIM_DESCRIPTION);
        // find the new description from the message
        const auto& pageData = message.page_data().components();
        for (const auto& comp : pageData) { 
            auto* submitEvent = event.mutable_submit_edit_claim_description();
            if (comp.id() == "editDescriptionInput") {
                submitEvent->set_new_description(comp.value());
                Log::debug("  New Description: " + comp.value());
            }
        }
    } else if (componentId == "cancelEditDescriptionButton" && eventType == "onClick") {
        Log::debug("  CANCEL_EDIT_CLAIM_DESCRIPTION for user: " + user);
        event.set_type(debate_event::CANCEL_EDIT_CLAIM_DESCRIPTION);
    } else if (componentId.rfind("viewChildNodeButton_", 0) == 0 && eventType == "onClick") {
        std::string claimId = componentId.substr(strlen("viewChildNodeButton_"));
        Log::debug("  GO_TO_CLAIM for user: " + user + " to claim ID: " + claimId);
        event.set_type(debate_event::GO_TO_CLAIM); // Placeholder, replace with actual GO_TO_CLAIM type
        event.mutable_go_to_claim()->set_claim_id(claimId);

    // add child claim
    } else if (componentId == "addChildClaimButton" && eventType == "onClick") {
        Log::debug("  OPEN_ADD_CHILD_CLAIM for user: " + user);
        event.set_type(debate_event::OPEN_ADD_CHILD_CLAIM);
    } else if (componentId == "closeAddChildClaimButton" && eventType == "onClick") {
        Log::debug("  CLOSE_ADD_CHILD_CLAIM for user: " + user);
        event.set_type(debate_event::CLOSE_ADD_CHILD_CLAIM);
     } else if (componentId == "submitAddChildClaimButton" && eventType == "onClick") {
        Log::debug("  ADD_CHILD_CLAIM for user: " + user);
        event.set_type(debate_event::ADD_CHILD_CLAIM);
        // find the claim and description from the message
        const auto& pageData = message.page_data().components();
        for (const auto& comp : pageData) { 
            auto* submitEvent = event.mutable_add_child_claim();
            if (comp.id() == "descriptionInput") {
                submitEvent->set_description(comp.value());
                Log::debug("  Description: " + comp.value());
            }
            else if (comp.id() == "claimSentenceInput") {
                submitEvent->set_claim(comp.value());
                Log::debug("  Claim: " + comp.value());
            }
        }
     }
    else {
        Log::error("Unknown component/event combination on debate page: " 
                  + componentId + "/" + eventType);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    return event;
}