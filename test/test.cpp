#include "../../../src/gen/cpp/page.pb.h"
#include "../../../src/gen/cpp/event.pb.h"
#include <iostream>
using namespace std;

#include "../debate/main/EventHandler.h"

int main() {
    cout<<"kello"<<endl;
    // Create a Page message
    ui::Page page;
    page.set_title("My Test Page");

    debate::UIEvent event;
    event.set_action_id("action");
    string action = event.action_id();
    std::cout << action << "\n";

    // Print something from it
    std::cout << "Page title: " << page.title() << std::endl;
    return 0;
}
