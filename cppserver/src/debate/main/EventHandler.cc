#include "EventHandler.h"
#include "../DebateModerator.h"   // uses DebateDatabaseHandler internally
#include "./events/enterDebate.h"
#include "./events/goHome.h"
#include <iostream>
using namespace std;

EventHandler::EventHandler(const debate::UIEvent& evt) : evt_(evt) {}
EventHandler::EventHandler(debate::UIEvent&& evt) : evt_(move(evt)) {}

void EventHandler::handleClientMessage(const debate::ClientMessage& msg, const string& user) {
    cout << "\n========================================\n";
    cout << "📬 ClientMessage received\n";
    cout << "✅ ClientMessage parsed successfully!\n";
    
    // Log the event info
    cout << "\n--- Event Info ---\n";
    cout << "Component ID: " << msg.component_id() << "\n";
    cout << "Event Type: " << msg.event_type() << "\n";
    cout << "User: " << user << "\n";

    // Log page data
    if (msg.has_page_data()) {
      const auto& page_data = msg.page_data();
      cout << "\n--- Page Data ---\n";
      cout << "Page ID: " << page_data.page_id() << "\n";
      cout << "Components count: " << page_data.components_size() << "\n";
      
      cout << "\n--- All Components ---\n";
      for (int i = 0; i < page_data.components_size(); i++) {
        const auto& comp = page_data.components(i);
        cout << "  [" << i << "] id: \"" << comp.id() 
                  << "\" = \"" << comp.value() << "\"\n";
      }
    } else {
      cout << "⚠️ No page data included\n";
    }
    
    cout << "========================================\n\n";
}

void EventHandler::handleEvent(const string& user) {
    const string actionId = evt_.action_id();
    DebateModerator moderator; // internally uses DebateDatabaseHandler
    EventHandler::Log(cout);
    if (actionId == "submitTopic") {
        string topic;

        // cout << "[EventHandler] event data size: " << evt_.event_data_size() << "\n";
        // for (const auto& ed : evt_.event_data()) {
        //     cout << "[EventHandler] event data key: " << ed.key() << "\n";
        //     if (ed.value().has_text_value()) {
        //         cout << "[EventHandler] event data text value: " << ed.value().text_value() << "\n";
        //     }
        // }
        // // LOGGING ABOVE IS FOR DEBUGGING PURPOSES
        // 1️⃣ Try to find topic in evt_.data() (map<string, string>)
        auto it = evt_.data().find("topicInput");
        if (it != evt_.data().end()) {
            topic = it->second;
        }

        // 2️⃣ If not found, check evt_.event_data()
        if (topic.empty() && evt_.event_data_size() > 0) {
            for (const auto& ed : evt_.event_data()) {
                if (ed.key() == "topicInput" && ed.value().has_text_value()) {
                    topic = ed.value().text_value();
                    break;
                }
            }
        }

        // 3️⃣ Perform database action via DebateModerator
        if (!topic.empty()) {
            moderator.handleAddDebate(user, topic);
        } else {
            cerr << "[EventHandler] submitTopic event missing topicInput value\n";
        }

    } else if (actionId == "clearTopics") {
        moderator.handleClearDebates(user);
        cout << "[EventHandler] clearTopics action processed.\n";

    } else if (actionId == "goHome") {
        // Handle "goHome" action
        goHome(user);
        cout << "[EventHandler] goHome action processed for user: " << user << "\n";

    } else if (actionId == "enterTopic") {
        // Handle "enterTopic" action
        string topicID;
        
        // // 1️⃣ Try to find topicID in evt_.data() (map<string, string>)
        // auto it = evt_.data().find("data-topicID");
        // if (it != evt_.data().end()) {
        //     topicID = it->second;
        //     cout << "[EventHandler] Found topicID in data: " << topicID << "\n";
        // }

        // // 2️⃣ If not found, check evt_.event_data()
        // if (topicID.empty() && evt_.event_data_size() > 0) {
        //     for (const auto& ed : evt_.event_data()) {
        //         if (ed.key() == "data-topicID" && ed.value().has_text_value()) {
        //             topicID = ed.value().text_value();
        //             cout << "[EventHandler] Found topicID in event_data: " << topicID << "\n";
        //             break;
        //         }
        //     }
        // }

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

// ---------------- Logging helpers ----------------
void EventHandler::Log(ostream& os) const {
    os << "[UIEvent]"
       << " id="      << evt_.event_id()
       << " comp="    << evt_.component_id()
       << " type="    << EventTypeName(evt_.type())
       << " action="  << evt_.action_id()
       << " ts="      << evt_.timestamp()
       << "\n";

    if (!evt_.data().empty()) {
        os << "  data:\n";
        for (const auto& kv : evt_.data()) {
            os << "    [" << kv.first << "] = " << kv.second << "\n";
        }
    }

    if (evt_.event_data_size() > 0) {
        os << "  event_data (" << evt_.event_data_size() << " items):\n";
        for (const auto& ed : evt_.event_data()) {
            os << "    " << ed.key() << " = ";
            PrintEventValue(os, ed.value());
            os << "\n";
        }
    }

    if (!evt_.metadata().empty()) {
        os << "  metadata:\n";
        for (const auto& kv : evt_.metadata()) {
            os << "    [" << kv.first << "] = " << kv.second << "\n";
        }
    }
}

void EventHandler::PrintEventValue(ostream& os, const debate::EventValue& v) const {
    using V = debate::EventValue;
    switch (v.value_case()) {
        case V::kTextValue:    os << "text:\"" << v.text_value() << "\""; break;
        case V::kNumberValue:  os << "number:" << static_cast<long long>(v.number_value()); break;
        case V::kBooleanValue: os << "bool:" << (v.boolean_value() ? "true" : "false"); break;
        case V::kDecimalValue: os << "decimal:" << v.decimal_value(); break;
        case V::kBinaryValue:  os << "bytes(" << v.binary_value().size() << ")"; break;
        case V::kListValue: {
            os << "list:[";
            const auto& vals = v.list_value().values();
            for (int i = 0; i < vals.size(); ++i) {
                if (i) os << ", ";
                os << '"' << vals[i] << '"';
            }
            os << "]";
            break;
        }
        case V::VALUE_NOT_SET:
        default: os << "(unset)"; break;
    }
}

const char* EventHandler::EventTypeName(debate::EventType t) const {
    switch (t) {
        case debate::UNKNOWN:      return "UNKNOWN";
        case debate::CLICK:        return "CLICK";
        case debate::INPUT_CHANGE: return "INPUT_CHANGE";
        case debate::FORM_SUBMIT:  return "FORM_SUBMIT";
        case debate::NODE_ADDED:   return "NODE_ADDED";
        case debate::NODE_REMOVED: return "NODE_REMOVED";
        case debate::EDGE_ADDED:   return "EDGE_ADDED";
        case debate::EDGE_REMOVED: return "EDGE_REMOVED";
        case debate::ACTION:       return "ACTION";
    }
    return "UNKNOWN";
}
