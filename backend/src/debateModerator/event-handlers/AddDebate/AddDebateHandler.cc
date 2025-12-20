
#include "AddDebateHandler.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../database/handlers/DebateDatabaseHandler.h"
#include "../../../utils/pathUtils.h"

void AddDebateHandler::AddDebate(const std::string& debateTopic, const std::string& user) {
    DebateDatabaseHandler dbHandler(utils::getDatabasePath());

    std::cout << "[AddDebateHandler] AddDebate called for user: "
              << user << ", topic: " << debateTopic << std::endl;

    debate::Debate debateProto;
    debateProto.set_topic(debateTopic);
    
    // Add the first claim (root claim) with proper content
    auto* rootClaim = debateProto.add_claims();
    rootClaim->set_sentence(debateTopic);
    rootClaim->set_description("This is the main topic of the debate, and for now this will be the placeholder text.");
    rootClaim->set_parent(""); // Empty parent means this is a root claim
    
    std::cout << "[AddDebateHandler] Created Debate protobuf for topic: " << debateProto.topic() 
              << " with " << debateProto.claims_size() << " claims" << std::endl;
    int newId = dbHandler.addDebate(user, debateTopic);
    if (newId == -1) {
        std::cerr << "[AddDebateHandler] Failed to insert debate topic.\n";
    } else {
        std::cout << "[AddDebateHandler] Inserted debate with ID: " << newId << std::endl;
    }

    bool ok = false;
    std::string serialized = debateProto.SerializeAsString();
    std::vector<uint8_t> debateBytes(serialized.begin(), serialized.end());

    ok = dbHandler.updateDebateProtobuf(user, debateTopic, debateBytes);
}