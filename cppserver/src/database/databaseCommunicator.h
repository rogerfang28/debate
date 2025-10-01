#pragma once
#include <string>
#include <vector>

struct Debate {
    int id;
    std::string topic;
};

// Open/close DB
void openDB(const std::string& filename);
void closeDB();

// Create table (only needs to be called once)
void createDebateTable();

// Main API
int addDebate(const std::string& user, const std::string& topic);
void deleteDebateTopic(const std::string& topic, const std::string& user); // added declaration for deleteDebateTopic
std::vector<Debate> readDebates(const std::string& user);
