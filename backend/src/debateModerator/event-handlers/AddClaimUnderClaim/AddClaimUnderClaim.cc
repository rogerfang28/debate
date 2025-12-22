#include "AddClaimUnderClaim.h"
#include <iostream>
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"
#include "../../../database/handlers/DebateDatabaseHandler.h"
#include "../../../database/handlers/UserDatabaseHandler.h"
#include "../../../utils/pathUtils.h"

void AddClaimUnderClaimHandler::AddClaimUnderClaim(const std::string& claim_text, const std::string& connection_to_parent, const std::string& user) {
    // first find where the user is in the debate
    UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    // get user protobuf
    std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
    user::User userProto;
    userProto.ParseFromArray(userData.data(), userData.size());
    std::string debateID = userProto.engagement().debating_info().debate_id();
    std::string currentClaimID = userProto.engagement().debating_info().current_claim_id();

    // find the debate protobuf and update it
    DebateDatabaseHandler debateDbHandler(utils::getDatabasePath());
    std::vector<uint8_t> debateData = debateDbHandler.getDebateProtobuf(debateID);
    debate::Debate debateProto;
    debateProto.ParseFromArray(debateData.data(), debateData.size());

    // find the current claim to add under
    debate::Claim* parentClaim = nullptr;
    for (int i = 0; i < debateProto.claims_size(); i++) {
        if (debateProto.claims(i).id() == currentClaimID) {
            parentClaim = debateProto.mutable_claims(i);
            break;
        }
    }
    if (parentClaim == nullptr) {
        std::cerr << "Error: Parent claim not found for ID " << currentClaimID << "\n";
        return;
    }

    // create new claim
    debate::Claim* newClaim = debateProto.add_claims();
    std::string newClaimID = std::to_string(debateProto.num_items()); // simple unique ID
    newClaim->set_id(newClaimID);
    newClaim->set_sentence(claim_text);
    newClaim->set_connection_to_parent(connection_to_parent);
    newClaim->set_parent_id(currentClaimID);
    // increment total statements count
    debateProto.set_num_items(debateProto.num_items() + 1);

    // add new claim ID to parent's children list
    parentClaim->add_children_ids(newClaimID);

    // save updated debate protobuf back to database
    // serialize to vector<uint8_t>
    std::vector<uint8_t> serializedDebate(debateProto.ByteSizeLong());
    debateProto.SerializeToArray(serializedDebate.data(), serializedDebate.size());
    debateDbHandler.updateDebateProtobuf(user, debateID, serializedDebate);
    // log
    std::cout << "[AddClaimUnderClaimHandler] Added new claim under claim ID " << currentClaimID 
              << " in debate ID " << debateID << " for user " << user << "\n";
}