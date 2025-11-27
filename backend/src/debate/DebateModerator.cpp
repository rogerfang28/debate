#include "DebateModerator.h"
#include "./DebateDatabaseHandler.h"   // now use the new handler
#include "../../../src/gen/cpp/debate.pb.h"
#include "../utils/pathUtils.h"
#include <iostream>
#include <vector>

DebateModerator::DebateModerator()
    : dbHandler(utils::getDatabasePath()) // initialize handler with database
{
    std::cout << "[DebateModerator] Initialized.\n";
}

DebateModerator::~DebateModerator() {
    std::cout << "[DebateModerator] Destroyed.\n";
}

// Add a debate topic for a specific user
void DebateModerator::handleAddDebate(const std::string& user, const std::string& debateTopic) {
    std::cout << "[DebateModerator] handleAddDebate called for user: "
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
    
    std::cout << "[DebateModerator] Created Debate protobuf for topic: " << debateProto.topic() 
              << " with " << debateProto.claims_size() << " claims" << std::endl;
    int newId = dbHandler.addDebate(user, debateTopic);
    if (newId == -1) {
        std::cerr << "[DEBUG DebateModerator] Failed to insert debate topic.\n";
    } else {
        // std::cout << "[DEBUG DebateModerator] Inserted debate with ID: " << newId << std::endl;
    }
    // std::cout << "[DEBUG DebateModerator] Adding protobuf data for debate topic...\n";
    bool ok = false;
    std::string serialized = debateProto.SerializeAsString();
    std::vector<uint8_t> debateBytes(serialized.begin(), serialized.end());

    ok = dbHandler.updateDebateProtobuf(user, debateTopic, debateBytes);
    if (ok) {
        std::cout << "[DEBUG DebateModerator] Added protobuf data for debate topic.\n";
        // std::cout << "[DEBUG DebateModerator] Debate protobuf size: " << debateBytes.size() << " bytes\n" << "Debate Topic: " << debateProto.topic() << std::endl;
    } else {
        std::cerr << "[DEBUG DebateModerator] Failed to add protobuf data for debate topic.\n";
    }

    //  testing retrieval (it worked)
    // std::vector<uint8_t> bytes = dbHandler.getDebateProtobuf(user, debateTopic);
    // debate::Debate debateProto2;
    // if (debateProto2.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) {
    //     std::cout << "[DEBUG DebateModerator] Successfully parsed protobuf data from database. Topic: "
    //               << debateProto2.topic() << std::endl;
    // } else {
    //     std::cerr << "[DEBUG DebateModerator] Failed to parse protobuf data from database.\n";
    // }

    printCurrentDebates(user);
}

// Clear all debate topics for a user
void DebateModerator::handleClearDebates(const std::string& user) {
    std::cout << "[DebateModerator] handleClearDebates called for user: " << user << std::endl;
    bool ok = dbHandler.clearUserDebates(user);

    if (ok)
        std::cout << "[DebateModerator] All debates cleared for user: " << user << std::endl;
    else
        std::cerr << "[DebateModerator] Failed to clear debates for user: " << user << std::endl;
}

// Print all current debates for a user
void DebateModerator::printCurrentDebates(const std::string& user) {
    auto debates = dbHandler.getDebates(user);
    std::cout << "[DebateModerator] Current debates for user '" << user << "':\n";
    if (debates.empty()) {
        std::cout << " (none)\n";
        return;
    }

    for (const auto& row : debates) {
        std::cout << " - " << row.at("ID") << " : " << row.at("TOPIC") << std::endl;
    }
}
