#include "DebatePageEventParser.h"
#include <iostream>

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
        std::cout << "  GO_HOME for user: " << user << "\n";
    } else if (componentId == "goToParentButton" && eventType == "onClick") {
        std::cout << "  GO_TO_PARENT for user: " << user << "\n";
        event.set_type(debate_event::GO_TO_PARENT);
    } else if (componentId == "addChildClaimButton" && eventType == "onClick") {
        std::cout << "  ADD_CLAIM for user: " << user << "\n";
        event.set_type(debate_event::ADD_CHILD_CLAIM);
        event.mutable_add_child_claim()->set_claim("test claim text");
        event.mutable_add_child_claim()->set_connection_to_parent("supports");
    } else if (componentId == "deleteStatementButton" && eventType == "onClick") {
        std::cout << "  DELETE_CURRENT_STATEMENT for user: " << user << "\n";
        event.set_type(debate_event::DELETE_CURRENT_STATEMENT);
    } else if (componentId == "reportButton" && eventType == "onClick") {
        std::cout << "  REPORT_CLAIM for user: " << user << "\n";
        event.set_type(debate_event::REPORT_CLAIM);
    } else if (componentId == "editDescriptionButton" && eventType == "onClick") {
        std::cout << "  EDIT_DESCRIPTION for user: " << user << "\n";
    } else if (componentId.rfind("viewChildNodeButton_", 0) == 0 && eventType == "onClick") {
        std::string claimId = componentId.substr(strlen("viewChildNodeButton_"));
        std::cout << "  GO_TO_CLAIM for user: " << user << " to claim ID: " << claimId << "\n";
        event.set_type(debate_event::GO_TO_CLAIM); // Placeholder, replace with actual GO_TO_CLAIM type
        event.mutable_go_to_claim()->set_claim_id(claimId);
    } else {
        std::cerr << "Unknown component/event combination on debate page: " 
                  << componentId << "/" << eventType << "\n";
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    return event;
}