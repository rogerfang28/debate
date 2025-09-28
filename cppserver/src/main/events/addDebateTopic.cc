#include "../../database/databaseCommunicator.h"

// debateTopicHandler.cc
#include <string>
#include <iostream>

// Function declaration
void addDebateTopic(const std::string& debateTopic);

// Function definition
void addDebateTopic(const std::string& debateTopic) {
    // TODO: implement logic here
    std::cout << "Received debate topic: " << debateTopic << std::endl;
    openDB("debates.db");                // open or create DB file
    createDebateTable();                 // make sure table exists

    int newId = insertDebate(debateTopic);  // insert a new debate
    std::cout << "Inserted debate with ID: " << newId << std::endl;

    auto debates = readDebates();        // get all debates
    for (const auto& d : debates) {
        std::cout << d.id << " - " << d.topic << std::endl;
    }

    closeDB();                           // clean up
}

int main(){
    addDebateTopic("Cliamte Change");
    readDebates();
    // return 0;
}   