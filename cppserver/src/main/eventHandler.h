#pragma once
#include "../../../src/gen/cpp/event.pb.h"
#include <string>
#include <ostream>

class EventHandler {
public:
    // constructors
    explicit EventHandler(const debate::UIEvent& evt);
    EventHandler(debate::UIEvent&& evt);

    // logging for debugging
    void Log(std::ostream& os) const;

    // main logic dispatcher
    void handleEvent(const std::string& user);

private:
    debate::UIEvent evt_;   // <-- underscore, consistent everywhere

    // helpers
    void PrintEventValue(std::ostream& os, const debate::EventValue& v) const;
    const char* EventTypeName(debate::EventType t) const;
};
