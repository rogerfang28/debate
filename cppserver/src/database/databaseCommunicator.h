#ifndef DATABASE_COMMUNICATOR_H
#define DATABASE_COMMUNICATOR_H

#include <string>
#include <vector>

struct Debate {
    int id;
    std::string topic;
};

// Functions
void openDB(const std::string& filename);
void createDebateTable();
int insertDebate(const std::string& topic);
std::vector<Debate> readDebates();
std::vector<Debate> searchDebates(const std::string& keyword);
void closeDB();

#endif
