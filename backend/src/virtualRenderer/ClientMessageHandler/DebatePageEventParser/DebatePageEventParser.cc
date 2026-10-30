#include "DebatePageEventParser.h"

#include <cstring>
#include <map>
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
    // The AI chat: the Ask button, Ctrl+Enter in the box, or a quick prompt.
    } else if ((componentId == "askChatButton" && eventType == "onClick") ||
               (componentId == "chatQuestionInput" && eventType == "onSubmit")) {
        event.set_type(debate_event::ASK_CHAT);
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "chatQuestionInput") {
                event.mutable_ask_chat()->set_question(comp.value());
            }
        }
        Log::debug("  ASK_CHAT for user: " + std::to_string(user_id));
    } else if (componentId.rfind("chatQuickButton_", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::ASK_CHAT);
        event.mutable_ask_chat()->set_quick_prompt(componentId.substr(strlen("chatQuickButton_")));
        Log::debug("  ASK_CHAT (quick) for user: " + std::to_string(user_id));
    } else if (componentId == "clearChatButton" && eventType == "onClick") {
        event.set_type(debate_event::CLEAR_CHAT);
        Log::debug("  CLEAR_CHAT for user: " + std::to_string(user_id));
    // The two tabs at the top of a debate, and the Brief's own buttons.
    } else if (componentId == "briefTabButton" && eventType == "onClick") {
        event.set_type(debate_event::GO_TO_BRIEF);
        Log::debug("  GO_TO_BRIEF for user: " + std::to_string(user_id));
    } else if (componentId == "debateTabButton" && eventType == "onClick") {
        event.set_type(debate_event::GO_TO_OVERVIEW);
        Log::debug("  GO_TO_OVERVIEW (tab) for user: " + std::to_string(user_id));
    } else if (componentId == "generateBriefButton" && eventType == "onClick") {
        event.set_type(debate_event::GENERATE_BRIEF);
        Log::debug("  GENERATE_BRIEF for user: " + std::to_string(user_id));
    } else if (componentId.rfind("briefOpenClaim_", 0) == 0 && eventType == "onClick") {
        const std::string claimId = componentId.substr(strlen("briefOpenClaim_"));
        Log::debug("  OPEN_CLAIM_IN_DEBATE for user: " + std::to_string(user_id) + " claim ID: " + claimId);
        event.set_type(debate_event::OPEN_CLAIM_IN_DEBATE);
        event.mutable_select_claim()->set_claim_id(std::stoi(claimId));
    // A [#12] reference in the AI chat (data-claim-links): the same as the
    // brief's links -- the debate view, with that claim selected -- from
    // either tab.
    } else if (componentId.rfind("chatClaimLink_", 0) == 0 && eventType == "onClick") {
        const std::string claimId = componentId.substr(strlen("chatClaimLink_"));
        Log::debug("  OPEN_CLAIM_IN_DEBATE (chat) for user: " + std::to_string(user_id) + " claim ID: " + claimId);
        event.set_type(debate_event::OPEN_CLAIM_IN_DEBATE);
        event.mutable_select_claim()->set_claim_id(std::stoi(claimId));
    } else if (componentId == "tutorialNextButton" && eventType == "onClick") {
        event.set_type(debate_event::TUTORIAL_NEXT);
        Log::debug("  TUTORIAL_NEXT for user: " + std::to_string(user_id));
    // Prefix, not equality: the guide box shows an exit in the header, a
    // Finish when the walkthrough ends, and a "Leave tutorial" when a script
    // will not load. They all mean the same event but must be DIFFERENT
    // elements -- sharing one id put duplicate ids in the document, and the
    // second one became unreachable to anything addressing it by id.
    } else if (componentId.rfind("tutorialExitButton", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::TUTORIAL_EXIT);
        Log::debug("  TUTORIAL_EXIT for user: " + std::to_string(user_id));
    } else if (componentId == "cancelAiWorkButton" && eventType == "onClick") {
        event.set_type(debate_event::CANCEL_AI_WORK);
        Log::debug("  CANCEL_AI_WORK for user: " + std::to_string(user_id));
    } else if (componentId == "dismissAiNoticeButton" && eventType == "onClick") {
        event.set_type(debate_event::DISMISS_AI_NOTICE);
        Log::debug("  DISMISS_AI_NOTICE for user: " + std::to_string(user_id));
    } else if (componentId == "seeOverviewButton" && eventType == "onClick") {
        event.set_type(debate_event::GO_TO_OVERVIEW);
        Log::debug("  GO_TO_OVERVIEW for user: " + std::to_string(user_id));
    } else if (componentId == "stepViewTestButton" && eventType == "onClick") {
        event.set_type(debate_event::GO_TO_FULL_DEBATE_VIEW);
        Log::debug("  GO_TO_FULL_DEBATE_VIEW for user: " + std::to_string(user_id));
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
        auto* submitEvent = event.mutable_add_child_claim();

        // Claims and descriptions are collected BY SUFFIX, not in DOM order.
        //
        // Each row in the modal is claimSentenceInput<N> plus
        // descriptionInput<N>. Appending both in whatever order the DOM scan
        // produced would misalign them the moment a row is left blank or
        // removed -- a description would land on somebody else's claim. Keying
        // on the shared suffix makes the pairing independent of order and of
        // gaps.
        std::map<std::string, std::string> claimsBySuffix;
        std::map<std::string, std::string> descsBySuffix;

        for (const auto& comp : pageData) {
            if (comp.id().rfind("claimSentenceInput", 0) == 0) {
                claimsBySuffix[comp.id().substr(strlen("claimSentenceInput"))] = comp.value();
            } else if (comp.id().rfind("descriptionInput", 0) == 0) {
                descsBySuffix[comp.id().substr(strlen("descriptionInput"))] = comp.value();
            } else if (comp.id() == "linkTypeSelect") {
                submitEvent->set_basis(comp.value());
                Log::debug("  Basis: " + comp.value());
            } else if (comp.id() == "claimVoiceInput") {
                submitEvent->set_voice(comp.value());
                Log::debug("  Voice: " + comp.value());
            } else if (comp.id() == "claimSourceUrlInput") {
                submitEvent->set_source_url(comp.value());
            } else if (comp.id() == "claimSourceExcerptInput") {
                submitEvent->set_source_excerpt(comp.value());
            } else if (comp.id() == "claimSourceImageInput") {
                submitEvent->set_source_image_url(comp.value());
            }
        }

        // The unsuffixed row is the primary claim; the rest are extras, and
        // each carries its own description at the same index.
        submitEvent->set_claim(claimsBySuffix[""]);
        submitEvent->set_description(descsBySuffix[""]);
        Log::debug("  Claim: " + claimsBySuffix[""]);

        for (const auto& entry : claimsBySuffix) {
            if (entry.first.empty() || entry.second.empty()) continue;
            submitEvent->add_extra_claims(entry.second);
            // Padded rather than skipped, so index N of extra_descriptions
            // always belongs to index N of extra_claims.
            auto descIt = descsBySuffix.find(entry.first);
            submitEvent->add_extra_descriptions(
                descIt == descsBySuffix.end() ? std::string() : descIt->second);
            Log::debug("  Extra claim: " + entry.second);
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
        auto* submitEvent = event.mutable_submit_challenge_claim();

        // Same suffix pairing as ADD_CHILD_CLAIM above -- see the note there
        // for why order-based collection misaligns descriptions.
        std::map<std::string, std::string> sentencesBySuffix;
        std::map<std::string, std::string> challengeDescsBySuffix;

        for (const auto& comp : pageData) {
            if (comp.id().rfind("challengeSentenceInput", 0) == 0) {
                sentencesBySuffix[comp.id().substr(strlen("challengeSentenceInput"))] = comp.value();
            } else if (comp.id().rfind("challengeDescriptionInput", 0) == 0) {
                challengeDescsBySuffix[comp.id().substr(strlen("challengeDescriptionInput"))] = comp.value();
            } else if (comp.id() == "challengeLinkTypeSelect") {
                submitEvent->set_basis(comp.value());
                Log::debug("  Challenge basis: " + comp.value());
            } else if (comp.id() == "challengeVoiceInput") {
                submitEvent->set_voice(comp.value());
                Log::debug("  Challenge voice: " + comp.value());
            } else if (comp.id() == "challengeSourceUrlInput") {
                submitEvent->set_source_url(comp.value());
            } else if (comp.id() == "challengeSourceExcerptInput") {
                submitEvent->set_source_excerpt(comp.value());
            } else if (comp.id() == "challengeSourceImageInput") {
                submitEvent->set_source_image_url(comp.value());
            }
        }

        submitEvent->set_challenge_sentence(sentencesBySuffix[""]);
        submitEvent->set_challenge_description(challengeDescsBySuffix[""]);
        Log::debug("  Challenge sentence: " + sentencesBySuffix[""]);

        for (const auto& entry : sentencesBySuffix) {
            if (entry.first.empty() || entry.second.empty()) continue;
            submitEvent->add_extra_sentences(entry.second);
            auto descIt = challengeDescsBySuffix.find(entry.first);
            submitEvent->add_extra_descriptions(
                descIt == challengeDescsBySuffix.end() ? std::string() : descIt->second);
            Log::debug("  Extra challenge sentence: " + entry.second);
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
        std::string challengedClaimId = componentId.substr(strlen("viewChallengeButton_"));
        Log::debug("  GO_TO_CHALLENGE for user: " + std::to_string(user_id) + " to challenged claim ID: " + challengedClaimId);
        event.set_type(debate_event::GO_TO_CHALLENGE);
        event.mutable_go_to_challenge()->set_challenge_id(std::stoi(challengedClaimId));
    }
    else if (componentId.find("deleteChallengeButton_") == 0 && eventType == "onClick"){
        std::string challengeId = componentId.substr(strlen("deleteChallengeButton_"));
        Log::debug("  DELETE_CHALLENGE for user: " + std::to_string(user_id) + " challenge ID: " + challengeId);
        event.set_type(debate_event::DELETE_CHALLENGE);
        event.mutable_delete_challenge()->set_challenge_id(std::stoi(challengeId));
    }
    else if (componentId.find("endorseClaimButton_") == 0 && eventType == "onClick") {
        const std::string claimId = componentId.substr(strlen("endorseClaimButton_"));
        Log::debug("  ENDORSE_CLAIM for user: " + std::to_string(user_id) + " claim ID: " + claimId);
        event.set_type(debate_event::ENDORSE_CLAIM);
        event.mutable_endorse_claim()->set_claim_id(std::stoi(claimId));
    }
    else if (componentId.find("unendorseClaimButton_") == 0 && eventType == "onClick") {
        const std::string claimId = componentId.substr(strlen("unendorseClaimButton_"));
        Log::debug("  UNENDORSE_CLAIM for user: " + std::to_string(user_id) + " claim ID: " + claimId);
        event.set_type(debate_event::UNENDORSE_CLAIM);
        event.mutable_unendorse_claim()->set_claim_id(std::stoi(claimId));
    }
    else if (componentId.find("concedeChallengeButton_") == 0 && eventType == "onClick"){
        std::string challengeLinkId = componentId.substr(strlen("concedeChallengeButton_"));
        Log::debug("  CONCEDE_CHALLENGE for user: " + std::to_string(user_id) + " challenge_link_id: " + challengeLinkId);
        event.set_type(debate_event::CONCEDE_CHALLENGE);
        event.mutable_concede_challenge()->set_challenge_link_id(std::stoi(challengeLinkId));
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
    else if (componentId.rfind("fullDebateStepButton_", 0) == 0 && eventType == "onClick") {
        std::string stepId = componentId.substr(strlen("fullDebateStepButton_"));
        Log::debug("  GO_TO_STEP for user: " + std::to_string(user_id) + " step ID: " + stepId);
        event.set_type(debate_event::GO_TO_CLAIM);
        event.mutable_go_to_claim()->set_claim_id(std::stoi(stepId));
    }
    else if (componentId.find("goToClaim_") == 0 && eventType == "onClick") {
        std::string claimId = componentId.substr(strlen("goToClaim_"));
        Log::debug("  GO_TO_CLAIM (map) for user: " + std::to_string(user_id) + " claim ID: " + claimId);
        event.set_type(debate_event::GO_TO_CLAIM);
        event.mutable_go_to_claim()->set_claim_id(std::stoi(claimId));
    }
    else if (componentId.rfind("selectClaimNode_", 0) == 0 && eventType == "onClick") {
        std::string claimId = componentId.substr(strlen("selectClaimNode_"));
        Log::debug("  SELECT_CLAIM for user: " + std::to_string(user_id) + " claim ID: " + claimId);
        event.set_type(debate_event::SELECT_CLAIM);
        event.mutable_select_claim()->set_claim_id(std::stoi(claimId));
    }
    // A click on a relation pill in the graph. The id is the RELATION id.
    else if (componentId.rfind("selectLinkNode_", 0) == 0 && eventType == "onClick") {
        std::string linkId = componentId.substr(strlen("selectLinkNode_"));
        Log::debug("  SELECT_LINK for user: " + std::to_string(user_id) + " link ID: " + linkId);
        event.set_type(debate_event::SELECT_LINK);
        event.mutable_select_link()->set_link_id(std::stoi(linkId));
    }
    // The selected-argument panel's Challenge button. Its own id, not the
    // modal's addLinkToChallengeButton_, so the two are never on one page
    // with the same id; both mean "put this link into a challenge".
    else if (componentId.rfind("challengeSelectedLinkButton_", 0) == 0 && eventType == "onClick") {
        std::string linkId = componentId.substr(strlen("challengeSelectedLinkButton_"));
        Log::debug("  ADD_LINK_TO_BE_CHALLENGED (from panel) for user: " + std::to_string(user_id) + " link ID: " + linkId);
        event.set_type(debate_event::ADD_LINK_TO_BE_CHALLENGED);
        event.mutable_add_link_to_be_challenged()->set_link_id(std::stoi(linkId));
    }
    else {
        Log::error("Unknown component/event combination on debate page: " 
                  + componentId + "/" + eventType);
        event.set_type(debate_event::EVENT_KIND_UNSPECIFIED);
    }
    
    return event;
}