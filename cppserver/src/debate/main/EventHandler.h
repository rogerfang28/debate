#ifndef EVENTHANDLER_H
#define EVENTHANDLER_H

#include <string>
#include <iostream>

#include "../../../src/gen/cpp/eventData.pb.h"

class EventHandler {
public:
    EventHandler();                // Constructor
    ~EventHandler();               // Destructor

    void handleEvent(const eventData::EventData& evt, const std::string& user);  // Handles an event
};

#endif // EVENTHANDLER_H
