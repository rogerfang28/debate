#include "../../../database/databaseCommunicator.h"
#include <string>
#include <iostream>

// Add a debate topic for a specific user
void addDebateTopic(const std::string& user, const std::string& debateTopic) {

    int newId = addDebate(user, debateTopic);  
    if (newId == -1) {
        std::cerr << "[AddDebates] Failed to insert debate topic.\n";
    } else {
        std::cout << "[AddDebates] Inserted debate with ID: " << newId << std::endl;
    }

    // std::cout << "[AddDebates] did i get here";
    auto debates = readDebates(user);  
    std::cout << "[AddDebates] Current debates for user '" << user << "':\n";
    for (const auto& d : debates) {
        std::cout << "[AddDebates] " << d.id << " - " << d.topic << std::endl;
    }

}

// quick test
// int main() {
//     addDebateTopic("defaultUser", "Climate Change");
//     return 0;
// }
