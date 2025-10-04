#include <iostream>

#include "virtualRenderer.h"

class VirtualRenderer {
private:
    // idk
public:
    VirtualRenderer() {
        std::cout << "VirtualRenderer initialized.\n";
    }
    ~VirtualRenderer() {
        std::cout << "VirtualRenderer destroyed.\n"; // probably never called
    }

    std::string layoutGenerator(const std::string& user) {
        // Placeholder rendering logic
        return "<html><body><h1>Welcome, " + user + "!</h1></body></html>";
    }
};