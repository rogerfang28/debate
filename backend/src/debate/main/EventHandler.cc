#include "EventHandler.h"
#include "../DebateModerator.h"   // uses DebateDatabaseHandler internally
#include "./events/enterDebate.h"
#include "./events/goHome.h"
#include <iostream>
using namespace std;

void EventHandler::handleEvent(const event::Event& evt, const string& user) {
    const string& actionId = evt.action_id();
    
    cout << "[EventHandler] Processing action: " << actionId << "\n";

    DebateModerator moderator;

    // Handle different actions
    if (actionId == "submitTopic") {
        // Submit topic action
        string topic = getEventData(evt, "topicInput");
        
        if (!topic.empty()) {
            moderator.handleAddDebate(user, topic);
            cout << "[EventHandler] submitTopic action processed.\n";
        } else {
            cerr << "[EventHandler] submitTopic event missing topicInput value\n";
        }

    } else if (actionId == "clearTopics") {
        // Clear topics action
        moderator.handleClearDebates(user);
        cout << "[EventHandler] clearTopics action processed.\n";

    } else if (actionId == "goHome") {
        // Go home action
        goHome(user);
        cout << "[EventHandler] goHome action processed for user: " << user << "\n";

    } else if (actionId == "enterTopic") {
        // Handle "enterTopic" action
        string topicID = getEventData(evt, "topicID");
        
        if (!topicID.empty()) {
            cout << "[EventHandler] Entering debate with ID: " << topicID << "\n";
            enterDebate(user, topicID);
            cout << "[EventHandler] enterTopic action processed for user: " << user << "\n";
        } else {
            cerr << "[EventHandler] enterTopic event missing topicID\n";
        }

    } else {
        cerr << "[EventHandler] Unhandled actionId: " << actionId << "\n";
    }
}

// Helper function to extract data from event
string EventHandler::getEventData(const event::Event& evt, const string& key) const {
    // Try data map first
    auto it = evt.data().find(key);
    if (it != evt.data().end()) {
        return it->second;
    }
    
    // The data() map already checked above, so just return empty
    // (Remove the redundant loop that was treating map pairs as protobuf objects)
    
    return "";
}
