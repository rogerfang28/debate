#pragma once

#include <string>
#include <iostream>
#include "../../../src/gen/cpp/event.pb.h"  // for debate::Event

// ---------------------------------------------------------
// Class: EventHandler
// ---------------------------------------------------------
// Handles incoming Events (translated from ClientMessage by 
// VirtualRenderer) and dispatches actions like adding or 
// clearing debate topics.
// ---------------------------------------------------------
class EventHandler {
public:
    EventHandler() = default;

    // Main event dispatcher (called by VirtualRenderer with translated Event)
    void handleEvent(const event::Event& evt, const std::string& user);

private:
    // Helper to extract data from event by key
    std::string getEventData(const event::Event& evt, const std::string& key) const;
};
