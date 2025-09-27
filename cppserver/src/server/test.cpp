#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#define _WIN32_WINNT 0x0A00

#include "../../../src/gen/cpp/page.pb.h"
#include "../../../src/gen/cpp/event.pb.h"
#include "../main/eventHandler.h"
#include "httplib.h"
#include <iostream>
#include <windows.h>
using namespace std;

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
