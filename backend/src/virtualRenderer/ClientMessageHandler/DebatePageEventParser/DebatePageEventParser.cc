#include "DebatePageEventParser.h"
#include <iostream>
#include "../../../utils/Log.h"

debate_event::DebateEvent DebatePageEventParser::ParseDebatePageEvent(
    const std::string& componentId,
    const std::string& eventType,
    const int& user_id,
    const client_message::ClientMessage& message
) {
    debate_event::DebateEvent event;
    
    Log::debug("Parsing DebatePageEvent for user: " + std::to_string(user_id));
    if (componentId == "goHomeButton" && eventType == "onClick") {
        event.set_type(debate_event::GO_HOME);
        Log::debug("  GO_HOME for user: " + std::to_string(user_id));
    } else if (componentId == "goToParentButton" && eventType == "onClick") {
        Log::debug("  GO_TO_PARENT for user: " + std::to_string(user_id));
        event.set_type(debate_event::GO_TO_PARENT);
    } else if (componentId == "deleteStatementButton" && eventType == "onClick") {
        Log::debug("  DELETE_CURRENT_STATEMENT for user: " + std::to_string(user_id));
        event.set_type(debate_event::DELETE_CURRENT_STATEMENT);
    } else if (componentId.rfind("deleteChildClaimButton_", 0) == 0 && eventType == "onClick") {
        std::string claimId = componentId.substr(strlen("deleteChildClaimButton_"));
        Log::debug("  DELETE_CHILD_CLAIM for user: " + std::to_string(user_id) + " claim ID: " + claimId);
        event.set_type(debate_event::DELETE_CHILD_CLAIM); // Placeholder, replace with actual DELETE_CHILD_CLAIM type
        event.mutable_delete_child_claim()->set_claim_id(std::stoi(claimId));
    } else if (componentId == "reportButton" && eventType == "onClick") {
        Log::debug("  REPORT_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::REPORT_CLAIM);

    // edit description
    } else if (componentId == "editDescriptionButton" && eventType == "onClick") {
        Log::debug("  EDIT_DESCRIPTION for user: " + std::to_string(user_id));
        event.set_type(debate_event::START_EDIT_CLAIM_DESCRIPTION);
    } else if (componentId == "saveDescriptionButton" && eventType == "onClick") {
        Log::debug("  SUBMIT_EDIT_CLAIM_DESCRIPTION for user: " + std::to_string(user_id));
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
        Log::debug("  CANCEL_EDIT_CLAIM_DESCRIPTION for user: " + std::to_string(user_id));
        event.set_type(debate_event::CANCEL_EDIT_CLAIM_DESCRIPTION);
    } else if (componentId.rfind("viewChildNodeButton_", 0) == 0 && eventType == "onClick") {
        std::string claimId = componentId.substr(strlen("viewChildNodeButton_"));
        Log::debug("  GO_TO_CLAIM for user: " + std::to_string(user_id) + " to claim ID: " + claimId);
        event.set_type(debate_event::GO_TO_CLAIM); // Placeholder, replace with actual GO_TO_CLAIM type
        event.mutable_go_to_claim()->set_claim_id(std::stoi(claimId));

    // add child claim
    } else if (componentId == "addChildClaimButton" && eventType == "onClick") {
        Log::debug("  OPEN_ADD_CHILD_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::OPEN_ADD_CHILD_CLAIM);
    } else if (componentId == "closeAddChildClaimButton" && eventType == "onClick") {
        Log::debug("  CLOSE_ADD_CHILD_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::CLOSE_ADD_CHILD_CLAIM);
     } else if (componentId == "submitAddChildClaimButton" && eventType == "onClick") {
        Log::debug("  ADD_CHILD_CLAIM for user: " + std::to_string(user_id));
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
     
     // edit claim
    else if (componentId == "editClaimButton" && eventType == "onClick") {
        Log::debug("  START_EDIT_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::START_EDIT_CLAIM);
    } else if (componentId == "saveClaimButton" && eventType == "onClick") {
        Log::debug("  SUBMIT_EDIT_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::SUBMIT_EDIT_CLAIM);
        // find the new claim from the message
        const auto& pageData = message.page_data().components();
        for (const auto& comp : pageData) { 
            auto* submitEvent = event.mutable_submit_edit_claim();
            if (comp.id() == "editClaimInput") {
                submitEvent->set_new_claim(comp.value());
                Log::debug("  New Claim: " + comp.value());
            }
        }
    } else if (componentId == "cancelEditClaimButton" && eventType == "onClick") {
        Log::debug("  CANCEL_EDIT_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::CANCEL_EDIT_CLAIM);
    }

    // connect claims
    else if (componentId.find("connectFromClaimButton_") == 0 && eventType == "onClick") {
        Log::debug("  CONNECT_FROM_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::CONNECT_FROM_CLAIM);
        std::string fromClaimId = componentId.substr(strlen("connectFromClaimButton_"));
        event.mutable_connect_from_claim()->set_from_claim_id(std::stoi(fromClaimId));
    } else if (componentId.find("connectToClaimButton_") == 0 && eventType == "onClick") {
        Log::debug("  CONNECT_TO_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::CONNECT_TO_CLAIM);
        std::string toClaimId = componentId.substr(strlen("connectToClaimButton_"));
        event.mutable_connect_to_claim()->set_to_claim_id(std::stoi(toClaimId));
    } else if (componentId == "submitConnectClaimsButton" && eventType == "onClick") {
        Log::debug("  SUBMIT_CONNECT_CLAIMS for user: " + std::to_string(user_id));
        event.set_type(debate_event::SUBMIT_CONNECT_CLAIMS);
        // find the connection input from the page
        const auto& pageData = message.page_data().components();
        for (const auto& comp : pageData) { 
            auto* submitEvent = event.mutable_submit_connect_claims();
            if (comp.id() == "connectionInput") {
                submitEvent->set_connection(comp.value());
                Log::debug("  Connection: " + comp.value());
            }
        } 
    } else if ((componentId.rfind("cancelConnectClaimsButton", 0) == 0 || componentId == "closeConnectModalButton") && eventType == "onClick") {
        Log::debug("  CANCEL_CONNECT_CLAIMS for user: " + std::to_string(user_id));
        event.set_type(debate_event::CANCEL_CONNECT_CLAIMS);
    } else if (componentId.rfind("deleteLinkButton_", 0) == 0 && eventType == "onClick") {
        std::string linkIdStr = componentId.substr(strlen("deleteLinkButton_"));
        Log::debug("  DELETE_LINK for user: " + std::to_string(user_id) + " link ID string: " + linkIdStr);
        try {
            int linkId = std::stoi(linkIdStr);
            event.set_type(debate_event::DELETE_LINK);
            event.mutable_delete_link()->set_link_id(linkId);
        } catch (const std::exception& e) {
            Log::error("Failed to parse link ID: " + linkIdStr + " - " + e.what());
            event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
        }
    }
    else if (componentId == "challengeClaimButton" && eventType == "onClick") {
        Log::debug("  CHALLENGE_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::START_CHALLENGE_CLAIM);
    } 
    else if (componentId == "cancelChallengeButton" && eventType == "onClick") {
        Log::debug("  CANCEL_CHALLENGE_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::CANCEL_CHALLENGE_CLAIM);
    }
    else if (componentId.find("addClaimToChallengeButton_") == 0 && eventType == "onClick") {
        std::string claimId = componentId.substr(strlen("addClaimToChallengeButton_"));
        Log::debug("  ADD_CLAIM_TO_BE_CHALLENGED for user: " + std::to_string(user_id) + " claim ID: " + claimId);
        event.set_type(debate_event::ADD_CLAIM_TO_BE_CHALLENGED);
        event.mutable_add_claim_to_be_challenged()->set_claim_id(std::stoi(claimId));
    } 
    else if (componentId.find("addLinkToChallengeButton_") == 0 && eventType == "onClick") {
        std::string linkId = componentId.substr(strlen("addLinkToChallengeButton_"));
        Log::debug("  ADD_LINK_TO_BE_CHALLENGED for user: " + std::to_string(user_id) + " link ID: " + linkId);
        event.set_type(debate_event::ADD_LINK_TO_BE_CHALLENGED);
        event.mutable_add_link_to_be_challenged()->set_link_id(std::stoi(linkId));
    }
    else if (componentId == "openCreateChallengeButton" && eventType == "onClick") {
        Log::debug("  OPEN_ADD_CHALLENGE for user: " + std::to_string(user_id));
        event.set_type(debate_event::OPEN_ADD_CHALLENGE);
    }
    else if (componentId == "closeChallengeModalButton" && eventType == "onClick") {
        Log::debug("  CLOSE_ADD_CHALLENGE for user: " + std::to_string(user_id));
        event.set_type(debate_event::CANCEL_CHALLENGE_CLAIM);
    } 
    else if (componentId == "submitChallengeModalButton" && eventType == "onClick") {
        Log::debug("  SUBMIT_CHALLENGE_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::SUBMIT_CHALLENGE_CLAIM);
        // find the challenge claim from the message
        const auto& pageData = message.page_data().components();
        for (const auto& comp : pageData) { 
            auto* submitEvent = event.mutable_submit_challenge_claim();
            if (comp.id() == "challengeSentenceInput") {
                submitEvent->set_challenge_sentence(comp.value());
                Log::debug("  Challenge sentence: " + comp.value());
            }
        }
    }
    else if (componentId.find("removeClaimFromChallengeButton_") == 0 && eventType == "onClick") {
        std::string claimId = componentId.substr(strlen("removeClaimFromChallengeButton_"));
        Log::debug("  REMOVE_CLAIM_TO_BE_CHALLENGED for user: " + std::to_string(user_id) + " claim ID: " + claimId);
        event.set_type(debate_event::REMOVE_CLAIM_TO_BE_CHALLENGED);
        event.mutable_remove_claim_to_be_challenged()->set_claim_id(std::stoi(claimId));
    } 
    else if (componentId.find("removeLinkFromChallengeButton_") == 0 && eventType == "onClick") {
        std::string linkId = componentId.substr(strlen("removeLinkFromChallengeButton_"));
        Log::debug("  REMOVE_LINK_TO_BE_CHALLENGED for user: " + std::to_string(user_id) + " link ID: " + linkId);
        event.set_type(debate_event::REMOVE_LINK_TO_BE_CHALLENGED);
        event.mutable_remove_link_to_be_challenged()->set_link_id(std::stoi(linkId));
    }

    else if (componentId.find("viewChallengeButton_") == 0 && eventType == "onClick"){
        std::string challengeId = componentId.substr(strlen("viewChallengeButton_"));
        Log::debug("  GO_TO_CHALLENGE for user: " + std::to_string(user_id) + " to challenge ID: " + challengeId);
        event.set_type(debate_event::GO_TO_CHALLENGE);
        event.mutable_go_to_challenge()->set_challenge_id(std::stoi(challengeId));
    }
    else if (componentId.find("deleteChallengeButton_") == 0 && eventType == "onClick"){
        std::string challengeId = componentId.substr(strlen("deleteChallengeButton_"));
        Log::debug("  DELETE_CHALLENGE for user: " + std::to_string(user_id) + " challenge ID: " + challengeId);
        event.set_type(debate_event::DELETE_CHALLENGE);
        event.mutable_delete_challenge()->set_challenge_id(std::stoi(challengeId));
    }
    else if (componentId == "concedeChallengeButton" && eventType == "onClick"){
        Log::debug("  CONCEDE_CHALLENGE for user: " + std::to_string(user_id));
        event.set_type(debate_event::CONCEDE_CHALLENGE);
    }

    else if (componentId == "goToChallengedClaimButton" && eventType == "onClick") {
        Log::debug("  REFRESH_DEBATE_PAGE for user: " + std::to_string(user_id));
        event.set_type(debate_event::GO_TO_CHALLENGED_PARENT_CLAIM);
    }

    else if (componentId == "modifyClaimButton" && eventType == "onClick") {
        Log::debug(" START_MODIFICATION_OF_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::START_MODIFICATION_OF_CLAIM);
    }

    else if (componentId == "submitModifyClaimButton" && eventType == "onClick") {
        Log::debug(" SUBMIT_MODIFICATION_OF_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::SUBMIT_MODIFICATION_OF_CLAIM);
    }
    else if (componentId == "cancelModifyClaimButton" && eventType == "onClick") {
        Log::debug(" CANCEL_MODIFICATION_OF_CLAIM for user: " + std::to_string(user_id));
        event.set_type(debate_event::CANCEL_MODIFICATION_OF_CLAIM);
    }
    else {
        Log::error("Unknown component/event combination on debate page: " 
                  + componentId + "/" + eventType);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    return event;
}