#include "eventHandler.h"
#include <iostream>

EventHandler::EventHandler(const debate::UIEvent& evt) : evt_(evt) {}
EventHandler::EventHandler(debate::UIEvent&& evt) : evt_(std::move(evt)) {}

void EventHandler::Log(std::ostream& os) const {
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



void EventHandler::PrintEventValue(std::ostream& os, const debate::EventValue& v) {
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

const char* EventHandler::EventTypeName(debate::EventType t) {
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
