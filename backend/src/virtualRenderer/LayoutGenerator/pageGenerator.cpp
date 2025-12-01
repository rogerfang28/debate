// #include "pageGenerator.h"
// #include "../../../src/gen/cpp/layout.pb.h"
// #include "../../../src/gen/cpp/debate.pb.h"
// #include "../../../src/gen/cpp/user.pb.h"
// #include "../../database/handlers/DebateDatabaseHandler.h"
// #include "../../utils/pathUtils.h"
// #include "../../database/handlers/UserDatabaseHandler.h"
// #include "../pages/generators/homePage/HomePageGenerator.h"
// #include "../pages/generators/debatePage/DebateClaimPageGenerator.h"
// #include "../pages/generators/loginPage/LoginPageGenerator.h"

// #include <google/protobuf/text_format.h>
// #include <fstream>
// #include <sstream>
// #include <iostream>
// #include <filesystem>
// #include <windows.h>

// // Helper function to find a component by ID recursively
// ui::Component* findComponent(ui::Component* root, const std::string& id) {
//     if (root->id() == id) {
//         return root;
//     }
    
//     for (auto& child : *root->mutable_children()) {
//         ui::Component* found = findComponent(&child, id);
//         if (found) {
//             return found;
//         }
//     }
    
//     return nullptr;
// }

// // Helper function to update text content of a component
// void updateComponentText(ui::Component* root, const std::string& id, const std::string& newText) {
//     ui::Component* comp = findComponent(root, id);
//     if (comp) {
//         comp->set_text(newText);
//         std::cerr << "[PageGen] Updated component '" << id << "' with text: " << newText.substr(0, 50) << "..." << std::endl;
//     } else {
//         std::cerr << "[PageGen][WARN] Component '" << id << "' not found" << std::endl;
//     }
// }

// std::string generatePage(const std::string& user) {
//     UserDatabaseHandler userDbHandler(utils::getDatabasePath());
    
//     // Check if user exists
//     if (!userDbHandler.userExists(user)) {
//         // Create a new user with default state
//         user::User newUser;
//         newUser.set_username(user);
//         newUser.set_state(user::UserState::NONE);
//         newUser.set_debate_topic_id("");
        
//         std::vector<uint8_t> userData(newUser.ByteSizeLong());
//         newUser.SerializeToArray(userData.data(), userData.size());
//         userDbHandler.addUser(user, userData);
        
//         // std::cerr << "[PageGen] Created new user: " << user << std::endl;
//         return generateHomePage(user);
//     }
    
//     // Get user protobuf data
//     std::vector<uint8_t> userData = userDbHandler.getUserProtobuf(user);
//     if (userData.empty()) {
//         std::cerr << "[PageGen][WARN] User exists but no protobuf data found for: " << user << std::endl;
//         return generateHomePage(user);
//     }
    
//     // Parse user protobuf
//     user::User userProto;
//     if (!userProto.ParseFromArray(userData.data(), static_cast<int>(userData.size()))) {
//         std::cerr << "[PageGen][ERR] Failed to parse user protobuf for: " << user << std::endl;
//         return generateHomePage(user);
//     }
    
//     std::cerr << "[PageGen] User " << user << " state: " << userProto.state() 
//               << " (NONE=" << user::UserState::NONE << ", DEBATING=" << user::UserState::DEBATING << ")" << std::endl;
    
//     // Generate page based on user state
//     switch (userProto.state()) {
//         case user::UserState::NONE:
//             // std::cout << "[DEBUGGING RIGHT NOW] Generating home page for user with state NONE" << std::endl;
//             return generateHomePage(user);
            
//         case user::UserState::DEBATING:
//             // std::cout << "[DEBUGGING RIGHT NOW] Generating debate page for user with state DEBATING" << std::endl;
//             if (userProto.debate_topic_id().empty()) {
//                 std::cerr << "[PageGen][WARN] User is DEBATING but has no debate_topic_id set" << std::endl;
//                 return generateHomePage(user);
//             }
//             std::cerr << "[PageGen] Generating debate page for topic id: " << userProto.debate_topic_id() << std::endl;
//             return generateDebateClaimPage(user, userProto.debate_topic_id(), userProto.debate_topic_id());
            
//         default:
//             std::cerr << "[PageGen][WARN] Unknown user state: " << userProto.state() << std::endl;
//             return generateHomePage(user);
//     }
// }

// std::string generateHomePage(const std::string& user) {

//     ui::Page page = HomePageGenerator::GenerateHomePage();

//     // -------- Serialize final page --------
//     std::string page_bin;
//     if (!page.SerializeToString(&page_bin)) {
//         std::cerr << "[PageGen][ERR] Failed to serialize page\n";
//         return {};
//     }

//     std::cerr << "[PageGen] Generated page with size=" << page_bin.size() << " bytes\n";
//     return page_bin;
// }

// std::string generateDebateClaimPage(const std::string& user, const std::string& topicId, const std::string& curClaim) {
//     ui::Page page = DebateClaimPageGenerator::GenerateDebatePage(topicId, curClaim);

//     // -------- Serialize final page --------
//     std::string page_bin;
//     if (!page.SerializeToString(&page_bin)) {
//         std::cerr << "[PageGen][ERR] Failed to serialize page\n";
//         return {};
//     }

//     // std::cerr << "[PageGen] Generated page with size=" << page_bin.size() << " bytes\n";
//     return page_bin;
// }

// std::string generateSignInPage() {
//     ui::Page page = LoginPageGenerator::GenerateLoginPage();

//     // -------- Serialize page --------
//     std::string page_bin;
//     if (!page.SerializeToString(&page_bin)) {
//         std::cerr << "[PageGen][ERR] Failed to serialize sign-in page\n";
//         return {};
//     }

//     std::cerr << "[PageGen] Generated sign-in page with size=" << page_bin.size() << " bytes\n";
//     return page_bin;
// }