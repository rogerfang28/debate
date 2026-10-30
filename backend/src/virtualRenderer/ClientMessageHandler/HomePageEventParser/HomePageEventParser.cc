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

    // * Any row or button that opens a debate on its Brief tab: the featured
    // case, a latest move, a case row. openBrief_<where>_<debate id>.
    else if (componentId.rfind("openBrief_", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::OPEN_DEBATE_BRIEF);
        const std::string debateID = componentId.substr(componentId.rfind('_') + 1);
        event.mutable_enter_debate()->set_debate_id(std::atoi(debateID.c_str()));
        Log::debug("  OPEN_DEBATE_BRIEF: debate_id = " + debateID);
    }

    // * The tabs: homeTab_<home|explore|mine>, optionally _<anything> after
    // it so a second control for the same tab has an id of its own. The
    // header's tabs reach here from every page (ClientMessageParser).
    else if (componentId.rfind("homeTab_", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::GO_TO_HOME_TAB);
        std::string tab = componentId.substr(std::string("homeTab_").size());
        tab = tab.substr(0, tab.find('_'));
        event.mutable_home_tab()->set_tab(tab);
        Log::debug("  GO_TO_HOME_TAB: tab = " + tab);
    }

    // * Explore: search (the button, or Enter in the box), clear, filter, sort.
    else if ((componentId == "exploreSearchButton" && eventType == "onClick") ||
             (componentId == "exploreSearchInput" && eventType == "onSubmit")) {
        event.set_type(debate_event::EXPLORE_SEARCH);
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "exploreSearchInput") {
                event.mutable_explore_search()->set_query(comp.value());
                break;
            }
        }
        event.mutable_explore_search();
        Log::debug("  EXPLORE_SEARCH: query = '" + event.explore_search().query() + "'");
    }
    else if (componentId == "exploreClearSearch" && eventType == "onClick") {
        event.set_type(debate_event::EXPLORE_SEARCH);
        event.mutable_explore_search()->set_query("");
    }
    else if (componentId.rfind("exploreFilter_", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::EXPLORE_FILTER);
        const std::string value = componentId.substr(std::string("exploreFilter_").size());
        event.mutable_explore_option()->set_value(value == "all" ? "" : value);
    }
    else if (componentId.rfind("exploreSort_", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::EXPLORE_SORT);
        event.mutable_explore_option()->set_value(componentId.substr(std::string("exploreSort_").size()));
    }

    // * "Answer" under Waiting on you: answerChallenge_<debate id>_<claim id>.
    else if (componentId.rfind("answerChallenge_", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::OPEN_DEBATE_CLAIM);
        const std::string ids = componentId.substr(std::string("answerChallenge_").size());
        const size_t split = ids.find('_');
        event.mutable_open_debate_claim()->set_debate_id(std::atoi(ids.substr(0, split).c_str()));
        if (split != std::string::npos) {
            event.mutable_open_debate_claim()->set_claim_id(std::atoi(ids.substr(split + 1).c_str()));
        }
        Log::debug("  OPEN_DEBATE_CLAIM: " + ids);
    }

    // * A person's name: openProfile_<where>_<user id>.
    else if (componentId.rfind("openProfile_", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::OPEN_PROFILE);
        const std::string userId = componentId.substr(componentId.rfind('_') + 1);
        event.mutable_open_profile()->set_user_id(std::atoi(userId.c_str()));
        Log::debug("  OPEN_PROFILE: user_id = " + userId);
    }

    // * Your own profile: Change username and Delete account open an inline
    // step; Cancel / Keep closes it.
    else if ((componentId == "profileRenameButton" || componentId == "profileDeleteButton" ||
              componentId == "profileRenameCancel" || componentId == "profileDeleteCancel") &&
             eventType == "onClick") {
        event.set_type(debate_event::PROFILE_PANEL);
        event.mutable_explore_option()->set_value(
            componentId == "profileRenameButton" ? "rename" : componentId == "profileDeleteButton" ? "delete" : "");
    }
    else if ((componentId == "profileRenameSave" && eventType == "onClick") ||
             (componentId == "profileRenameInput" && eventType == "onSubmit")) {
        event.set_type(debate_event::RENAME_ACCOUNT);
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "profileRenameInput") {
                event.mutable_rename_account()->set_new_username(comp.value());
                break;
            }
        }
        event.mutable_rename_account();
        Log::debug("  RENAME_ACCOUNT: '" + event.rename_account().new_username() + "'");
    }
    else if (componentId == "profileDeleteConfirm" && eventType == "onClick") {
        event.set_type(debate_event::DELETE_ACCOUNT);
    }

    // * An admin's "Import debate" on their own profile. Preview and Publish
    // carry the import box's text; the moderator ignores all four from
    // anyone not in DEBATE_ADMINS.
    else if (componentId == "profileImportButton" && eventType == "onClick") {
        event.set_type(debate_event::IMPORT_OPEN);
    }
    else if (componentId == "importCloseButton" && eventType == "onClick") {
        event.set_type(debate_event::IMPORT_CLOSE);
    }

    // * An admin's "Activity" on their own profile: open, close, and the
    // recent list's filter, activityFilter_<all|members|visitors>, optionally
    // _<anything> after it (Refresh is the current filter again). The
    // moderator ignores all three from anyone not in DEBATE_ADMINS.
    else if (componentId == "profileActivityButton" && eventType == "onClick") {
        event.set_type(debate_event::ACTIVITY_OPEN);
    }
    else if (componentId == "activityCloseButton" && eventType == "onClick") {
        event.set_type(debate_event::ACTIVITY_CLOSE);
    }
    else if (componentId.rfind("activityFilter_", 0) == 0 && eventType == "onClick") {
        event.set_type(debate_event::ACTIVITY_FILTER);
        std::string value = componentId.substr(std::string("activityFilter_").size());
        event.mutable_explore_option()->set_value(value.substr(0, value.find('_')));
    }
    else if ((componentId == "importPreviewButton" || componentId == "importPublishButton") &&
             eventType == "onClick") {
        event.set_type(componentId == "importPreviewButton" ? debate_event::IMPORT_PREVIEW
                                                            : debate_event::IMPORT_PUBLISH);
        event.mutable_import_debate();
        for (const auto& comp : message.page_data().components()) {
            if (comp.id() == "importText") {
                event.mutable_import_debate()->set_text(comp.value());
                break;
            }
        }
        Log::debug("  " + componentId + ": " + std::to_string(event.import_debate().text().size()) + " bytes");
    }

    // * "Start a debate" opens the create dialog. A visitor's press is
    // remembered by the moderator and replayed after sign-in, so they land
    // in the open dialog as themselves.
    else if (componentId == "startDebateButton" && eventType == "onClick") {
        event.set_type(debate_event::OPEN_CREATE_DEBATE);
    }
    else if (componentId == "closeCreateDebateButton" && eventType == "onClick") {
        event.set_type(debate_event::CLOSE_CREATE_DEBATE);
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