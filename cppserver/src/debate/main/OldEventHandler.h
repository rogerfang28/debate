#ifndef OLDEVENTHANDLER_H
#define OLDEVENTHANDLER_H

#include <string>
#include <iostream>
#include "../../../src/gen/cpp/event.pb.h"  // for debate::UIEvent, EventType, EventValue

// ---------------------------------------------------------
// Class: EventHandler
// ---------------------------------------------------------
// Handles incoming UI events (like button clicks or form
// submissions) from the frontend and dispatches actions
// like adding or clearing debate topics.
// ---------------------------------------------------------
class OldEventHandler {
public:
    // Constructors for lvalue and rvalue references
    explicit OldEventHandler(const debate::UIEvent& evt);
    explicit OldEventHandler(debate::UIEvent&& evt);

    // Main event dispatcher (called by backend)
    void handleEvent(const std::string& user);

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
