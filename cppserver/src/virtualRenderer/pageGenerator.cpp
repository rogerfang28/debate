#include "pageGenerator.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../../src/gen/cpp/debate.pb.h"
#include "../debate/DebateDatabaseHandler.h"
#include "../utils/pathUtils.h"
#include "../debate/UserDatabaseHandler.h"

#include <google/protobuf/text_format.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <windows.h>

// Helper function to find a component by ID recursively
ui::Component* findComponent(ui::Component* root, const std::string& id) {
    if (root->id() == id) {
        return root;
    }
    
    for (auto& child : *root->mutable_children()) {
        ui::Component* found = findComponent(&child, id);
        if (found) {
            return found;
        }
    }
    
    return nullptr;
}

// Helper function to update text content of a component
void updateComponentText(ui::Component* root, const std::string& id, const std::string& newText) {
    ui::Component* comp = findComponent(root, id);
    if (comp) {
        comp->set_text(newText);
        std::cerr << "[PageGen] Updated component '" << id << "' with text: " << newText.substr(0, 50) << "..." << std::endl;
    } else {
        std::cerr << "[PageGen][WARN] Component '" << id << "' not found" << std::endl;
    }
}

std::string generatePage(const std::string& user) {
    // This should get the location of the user from the database, then use generateTestPage or generateDebateClaimPage
    UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    auto rows = userDbHandler.getUser(user);
    std::string location = "home"; // default
    if (!rows.empty()) {
        location = rows[0]["LOCATION"];
    }
    else{
        userDbHandler.addUser(user, "Default Name", "2024-01-01", "home");
    }
    if (location == "home") {
        return generateTestPage(user);
    } else {
        // for now assume that if not home, then debate page and location = topic
        std::string topicID = location; // This would be dynamic in a real scenario
        // now I need to get the topic from the topicID
        std::string curClaim = topicID; // This would also be dynamic
        return generateDebateClaimPage(user, topicID, curClaim);
    }
}

std::string generateTestPage(const std::string& user) {
    // -------- Get absolute path to pbtxt --------
    std::filesystem::path exeDir = utils::getExeDir();

    // Navigate relative to exe location (usually cppserver/build/bin/)
    std::filesystem::path pbtxtPath =
        exeDir / ".." / ".." / "src" / "virtualRenderer" / "pages" / "schemas" / "testPage.pbtxt";

    pbtxtPath = std::filesystem::weakly_canonical(pbtxtPath);

    std::cerr << "[PageGen] Looking for pbtxt at " << pbtxtPath.u8string() << "\n";

    std::ifstream input(pbtxtPath, std::ios::binary);
    if (!input) {
        std::cerr << "[PageGen][ERR] Couldn't open " << pbtxtPath.u8string() << "\n";
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    ui::Page page;
    if (!google::protobuf::TextFormat::ParseFromString(buffer.str(), &page)) {
        std::cerr << "[PageGen][ERR] TextFormat parse failed\n";
        return {};
    }

    // -------- Fetch debates for user --------
    DebateDatabaseHandler dbHandler(utils::getDatabasePath());
    auto debates = dbHandler.getDebates(user);

    std::cerr << "[PageGen] getDebates(\"" << user << "\") returned "
              << debates.size() << " items\n";
    for (const auto& d : debates) {
        std::cerr << "    id=" << d.at("ID") << " topic=\"" << d.at("TOPIC") << "\"\n";
    }

    // -------- Find topicsList component --------
    ui::Component* topicsList = nullptr;
    if (page.components_size() > 0) {
        auto* root = page.mutable_components(0);
        for (auto& comp : *root->mutable_children()) {
            if (comp.id() == "topicsCard") {
                for (auto& child : *comp.mutable_children()) {
                    if (child.id() == "topicsList") {
                        topicsList = &child;
                        break;
                    }
                }
            }
        }
    }

    if (!topicsList) {
        std::cerr << "[PageGen][WARN] topicsList component not found in page schema\n";
    } else {
        std::cerr << "[PageGen] Injecting " << debates.size()
                  << " debate topics into topicsList\n";

        // Change the component type to CONTAINER so we can add children
        topicsList->set_type(ui::ComponentType::CONTAINER);
        topicsList->clear_children();
        
        for (const auto& row : debates) {
            std::cerr << "[PageGen] Adding topic: " << row.at("TOPIC") << " with ID: " << row.at("ID") << std::endl;
            
            // Create a container for each topic row (topic text + status)
            auto* topicRow = topicsList->add_children();
            topicRow->set_id("topicRow_" + row.at("ID"));
            topicRow->set_type(ui::ComponentType::CONTAINER);
            topicRow->mutable_style()->set_custom_class("flex items-center justify-between p-3 border-b border-gray-200 hover:bg-gray-50");
            
            // Topic text
            auto* topicText = topicRow->add_children();
            topicText->set_id("topicText_" + row.at("ID"));
            topicText->set_type(ui::ComponentType::TEXT);
            topicText->set_text(row.at("TOPIC"));
            topicText->mutable_style()->set_custom_class("flex-1 text-gray-800");
            
            // Status indicator (instead of interactive button)
            auto* statusIndicator = topicRow->add_children();
            statusIndicator->set_id("statusIndicator_" + row.at("ID"));
            statusIndicator->set_type(ui::ComponentType::TEXT);
            statusIndicator->set_text("Available");
            statusIndicator->mutable_style()->set_custom_class("px-3 py-1 bg-green-100 text-green-800 rounded-full text-xs font-medium");
        }
        
        std::cerr << "[PageGen] Successfully added " << debates.size() << " topic components" << std::endl;
    }

    // -------- Serialize final page --------
    std::string page_bin;
    if (!page.SerializeToString(&page_bin)) {
        std::cerr << "[PageGen][ERR] Failed to serialize page\n";
        return {};
    }

    std::cerr << "[PageGen] Generated page with size=" << page_bin.size() << " bytes\n";
    return page_bin;
}

std::string generateDebateClaimPage(const std::string& user, const std::string& topic, const std::string& curClaim) {
    // -------- Get absolute path to pbtxt --------
    std::filesystem::path exeDir = utils::getExeDir();

    // Navigate relative to exe location (usually cppserver/build/bin/)
    std::filesystem::path pbtxtPath =
        exeDir / ".." / ".." / "src" / "virtualRenderer" / "pages" / "schemas" / "debatePage.pbtxt";

    pbtxtPath = std::filesystem::weakly_canonical(pbtxtPath);

    std::cerr << "[PageGen] Looking for pbtxt at " << pbtxtPath.u8string() << "\n";

    std::ifstream input(pbtxtPath, std::ios::binary);
    if (!input) {
        std::cerr << "[PageGen][ERR] Couldn't open " << pbtxtPath.u8string() << "\n";
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    ui::Page page;
    if (!google::protobuf::TextFormat::ParseFromString(buffer.str(), &page)) {
        std::cerr << "[PageGen][ERR] TextFormat parse failed\n";
        return {};
    }

    // change it based on user/topic/curClaim
    // -------- Fetch debate claims for user & topic --------
    DebateDatabaseHandler dbHandler(utils::getDatabasePath());
    
    std::vector<uint8_t> bytes = dbHandler.getDebateProtobuf(user, topic); // so right here it's not returning correctly
    debate::Debate debateProto;
    if (debateProto.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) {
        std::cout << "[PageGen] Successfully parsed protobuf data from database. Topic: "
                  << debateProto.topic() << std::endl;
    } else {
        std::cerr << "[PageGen][ERR] Failed to parse protobuf data from database.\n";
    }

    std::cerr << "[PageGen] Debate protobuf for topic: " << debateProto.topic() 
              << " with " << debateProto.claims_size() << " claims" << std::endl;

    // -------- Find the current claim --------
    const debate::Claim* currentClaim = nullptr;
    for (int i = 0; i < debateProto.claims_size(); ++i) {
        if (debateProto.claims(i).sentence() == curClaim) {
            currentClaim = &debateProto.claims(i);
            std::cout << "[PageGen] Found current claim: " << currentClaim->sentence() << std::endl;
            break;
        }
    }
    // -------- Update page components with debate data --------
    if (currentClaim && page.components_size() > 0) {
        auto* mainLayout = page.mutable_components(0);
        
        // Update current claim title in focus section
        updateComponentText(mainLayout, "currentClaimTitle", currentClaim->sentence());
        
        // Update description content
        updateComponentText(mainLayout, "descriptionContent", currentClaim->description());
        
        // Find and populate child arguments
        auto* childArgumentsGrid = findComponent(mainLayout, "childArgumentsGrid");
        if (childArgumentsGrid) {
            childArgumentsGrid->clear_children();
            
            // Find all child claims
            std::vector<const debate::Claim*> childClaims;
            for (const std::string& childSentence : currentClaim->children()) {
                for (int i = 0; i < debateProto.claims_size(); ++i) {
                    if (debateProto.claims(i).sentence() == childSentence) {
                        childClaims.push_back(&debateProto.claims(i));
                        break;
                    }
                }
            }
            
            std::cerr << "[PageGen] Found " << childClaims.size() << " child claims" << std::endl;
            
            // Create child argument components
            for (size_t i = 0; i < childClaims.size(); ++i) {
                const auto* childClaim = childClaims[i];
                
                auto* childContainer = childArgumentsGrid->add_children();
                childContainer->set_id("childNode" + std::to_string(i + 1));
                childContainer->set_type(ui::ComponentType::CONTAINER);
                childContainer->mutable_style()->set_custom_class("bg-gray-50 border border-gray-300 rounded p-4");
                
                // Child claim title
                auto* childTitle = childContainer->add_children();
                childTitle->set_id("childNode" + std::to_string(i + 1) + "Title");
                childTitle->set_type(ui::ComponentType::TEXT);
                childTitle->set_text(childClaim->sentence());
                childTitle->mutable_style()->set_custom_class("text-gray-700 mb-3 text-sm");
                
                // Child claim description (if available)
                if (!childClaim->description().empty()) {
                    auto* childDesc = childContainer->add_children();
                    childDesc->set_id("childNode" + std::to_string(i + 1) + "Desc");
                    childDesc->set_type(ui::ComponentType::TEXT);
                    childDesc->set_text(childClaim->description());
                    childDesc->mutable_style()->set_custom_class("text-gray-600 mb-3 text-xs");
                }
                
                // View indicator (instead of navigation button)
                auto* viewIndicator = childContainer->add_children();
                viewIndicator->set_id("childNode" + std::to_string(i + 1) + "Indicator");
                viewIndicator->set_type(ui::ComponentType::TEXT);
                viewIndicator->set_text("Child Argument #" + std::to_string(i + 1));
                viewIndicator->mutable_style()->set_custom_class("w-full px-3 py-2 bg-blue-100 text-blue-800 rounded text-center text-sm font-medium");
            }
            
            // If no child claims, show a message
            if (childClaims.empty()) {
                auto* noChildrenMsg = childArgumentsGrid->add_children();
                noChildrenMsg->set_id("noChildrenMessage");
                noChildrenMsg->set_type(ui::ComponentType::TEXT);
                noChildrenMsg->set_text("No child arguments for this claim.");
                noChildrenMsg->mutable_style()->set_custom_class("text-gray-500 italic text-center col-span-3");
            }
        }
        
        // Update parent display (instead of interactive button)
        if (!currentClaim->parent().empty()) {
            auto* parentInfo = findComponent(mainLayout, "goToParentButton");
            if (parentInfo) {
                parentInfo->set_type(ui::ComponentType::TEXT);
                parentInfo->set_text("Parent: " + currentClaim->parent().substr(0, 50) + "...");
                parentInfo->mutable_style()->set_custom_class("px-4 py-2 bg-gray-100 text-gray-700 rounded text-sm border");
            }
        } else {
            // Show root status
            auto* parentInfo = findComponent(mainLayout, "goToParentButton");
            if (parentInfo) {
                parentInfo->set_type(ui::ComponentType::TEXT);
                parentInfo->set_text("Root Claim (No Parent)");
                parentInfo->mutable_style()->set_custom_class("px-4 py-2 bg-gray-100 text-gray-500 rounded text-sm border");
            }
        }
    }

    // -------- Serialize final page --------
    std::string page_bin;
    if (!page.SerializeToString(&page_bin)) {
        std::cerr << "[PageGen][ERR] Failed to serialize page\n";
        return {};
    }

    std::cerr << "[PageGen] Generated page with size=" << page_bin.size() << " bytes\n";
    return page_bin;
}