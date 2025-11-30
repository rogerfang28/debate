
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
    
    // // Add a supporting claim
    // auto* supportingClaim = debateProto.add_claims();
    // supportingClaim->set_sentence("Transparency builds public trust in AI systems");
    // supportingClaim->set_description("When people understand how AI works, they are more likely to accept and use it");
    // supportingClaim->set_parent("AI systems should be transparent and explainable"); // Reference to parent
    
    // // Add a counter-argument
    // auto* counterClaim = debateProto.add_claims();
    // counterClaim->set_sentence("Complete transparency may reveal trade secrets");
    // counterClaim->set_description("Companies invest heavily in AI research and need some protection");
    // counterClaim->set_parent("AI systems should be transparent and explainable"); // Same parent
    
    // // Update the root claim to reference its children
    // rootClaim->add_children("Transparency builds public trust in AI systems");
    // rootClaim->add_children("Complete transparency may reveal trade secrets");
    // above is a bunch of example claims, maybe I'll try to use them
    
    std::cout << "[AddDebateHandler] Created Debate protobuf for topic: " << debateProto.topic() 
              << " with " << debateProto.claims_size() << " claims" << std::endl;
    int newId = dbHandler.addDebate(user, debateTopic);
    if (newId == -1) {
        std::cerr << "[AddDebateHandler] Failed to insert debate topic.\n";
    } else {
        // std::cout << "[DEBUG DebateModerator] Inserted debate with ID: " << newId << std::endl;
    }
    // std::cout << "[DEBUG DebateModerator] Adding protobuf data for debate topic...\n";
    bool ok = false;
    std::string serialized = debateProto.SerializeAsString();
    std::vector<uint8_t> debateBytes(serialized.begin(), serialized.end());

    ok = dbHandler.updateDebateProtobuf(user, debateTopic, debateBytes);

    //  testing retrieval (it worked)
    // std::vector<uint8_t> bytes = dbHandler.getDebateProtobuf(user, debateTopic);
    // debate::Debate debateProto2;
    // if (debateProto2.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) {
    //     std::cout << "[DEBUG DebateModerator] Successfully parsed protobuf data from database. Topic: "
    //               << debateProto2.topic() << std::endl;
    // } else {
    //     std::cerr << "[DEBUG DebateModerator] Failed to parse protobuf data from database.\n";
    // }
}