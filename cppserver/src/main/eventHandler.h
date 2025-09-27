#pragma once
#include <iosfwd>
#include "../../../src/gen/cpp/event.pb.h"  // debate::UIEvent, EventValue, EventType

// Per-request handler that owns a copy of the event.
class EventHandler {
public:
    explicit EventHandler(const debate::UIEvent& evt);   // copy constructor
    explicit EventHandler(debate::UIEvent&& evt);        // move constructor
    EventHandler(const EventHandler&) = default;
    EventHandler(EventHandler&&) = default;
    EventHandler& operator=(const EventHandler&) = default;
    EventHandler& operator=(EventHandler&&) = default;
    ~EventHandler() = default;

    // Log the event to a stream (default stdout)
    void Log(std::ostream& os = std::cout) const;
    void HandleEvent();

    // Optionally expose the event (read-only)
    const debate::UIEvent& event() const { return evt_; }

private:
    static void PrintEventValue(std::ostream& os, const debate::EventValue& v);
    static const char* EventTypeName(debate::EventType t);

    debate::UIEvent evt_;  // owned copy (thread-safe per instance)
};
