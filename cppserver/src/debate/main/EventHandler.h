#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <string>
#include <iostream>
#include "../../../src/gen/cpp/event.pb.h"  // for debate::UIEvent, EventType, EventValue
#include "../../../src/gen/cpp/client_message.pb.h"  // for debate::ClientMessage

// ---------------------------------------------------------
// Class: EventHandler
// ---------------------------------------------------------
// Handles incoming UI events (like button clicks or form
// submissions) from the frontend and dispatches actions
// like adding or clearing debate topics.
// ---------------------------------------------------------
class EventHandler {
public:
    // Constructors for lvalue and rvalue references
    explicit EventHandler(const debate::UIEvent& evt);
    explicit EventHandler(debate::UIEvent&& evt);
    EventHandler() = default;  // Default constructor

    // Main event dispatcher (called by backend)
    void handleEvent(const std::string& user);
    void handleClientMessage(const debate::ClientMessage& msg, const std::string& user);

    // Logging and debug helpers
    void Log(std::ostream& os) const;

private:
    debate::UIEvent evt_;  // The event payload

    // Helper to print a single EventValue (for debugging/logs)
    void PrintEventValue(std::ostream& os, const debate::EventValue& v) const;

    // Converts EventType enum to readable string
    const char* EventTypeName(debate::EventType t) const;
};

#endif // EVENTHANDLER_H
