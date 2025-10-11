#include "EventHandler.h"
using namespace std;


EventHandler::EventHandler() {
    std::cout << "[EventHandler] Initialized.\n";
}

EventHandler::~EventHandler() {
    std::cout << "[EventHandler] Destroyed.\n";
}

void EventHandler::handleEvent(const eventData::EventData& evt, const string& user) {
    string actionId = evt.actionid(); // Example usage of evt
    std::cout << "[EventHandler] Handling event with actionId: " << actionId << " for user: " << user << std::endl;

    // You can add conditional logic here:
    if (actionId == "submitTopic") {
        std::cout << "[EventHandler] submitTopic\n";
    } else if (actionId == "clearTopics") {
        std::cout << "[EventHandler] clearTopics\n";
    } else if (actionId == "goHome") {
        std::cout << "[EventHandler] goHome\n";
    } else if (actionId == "enterTopic") {  
        std::cout << "[EventHandler] enterTopic\n";
    } else {
        std::cout << "[EventHandler] Unknown event.\n";
    }
}
