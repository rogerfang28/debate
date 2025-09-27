#include <iostream>
#include "databaseCommunicator.h"

int main() {
    openDB("debates.sqlite3");
    createDebateTable();

    int id = insertDebate("Is technology making us smarter or lazier?");
    std::cout << "Inserted debate with ID " << id << "\n";

    auto debates = readDebates();
    std::cout << "All debates:\n";
    for (const auto& d : debates) {
        std::cout << "[" << d.id << "] " << d.topic << "\n";
    }

    closeDB();
    return 0;
}
