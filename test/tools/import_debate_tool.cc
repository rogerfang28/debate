// import_debate_tool.cc -- replay a scenario file into a database
//
// The replay logic itself lives in backend/src/utils/ScenarioImporter, because
// AI debate generation replays scenarios too and two copies would drift.

#include <cstdlib>
#include <iostream>
#include <string>

#include "database/sqlite/Database.h"
#include "debateModerator/DebateModerator.h"
#include "utils/Log.h"
#include "utils/ScenarioImporter.h"
#include "utils/pathUtils.h"

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: import_debate_tool <scenario.pbtxt> [db_path] [users_db_path]" << std::endl;
        std::cerr << std::endl;
        std::cerr << "  db_path        debates database. Defaults to DB_PATH, then the built-in path." << std::endl;
        std::cerr << "  users_db_path  users database. Defaults to db_path." << std::endl;
        std::cerr << std::endl;
        std::cerr << "  Production splits these (backend/debates.sqlite3 + backend/users.sqlite3)." << std::endl;
        std::cerr << "  Importing into a live database WITHOUT users_db_path creates the scenario's" << std::endl;
        std::cerr << "  users in the debates file, where the server never looks -- so claims end up" << std::endl;
        std::cerr << "  attributed to whoever happens to hold that id in the real users file." << std::endl;
        return 1;
    }

    const std::string scenario_path = argv[1];
    if (argc >= 3) {
        _putenv_s("DB_PATH", argv[2]);
    }
    const std::string users_db_path = (argc >= 4) ? argv[3] : utils::getDatabasePath();

    Log::init(LogLevel::Warn);

    bool loaded_ok = false;
    debate_test::TestScenario scenario = scenario::LoadFromFile(scenario_path, loaded_ok);
    if (!loaded_ok) {
        return 1;
    }
    if (scenario.steps_size() == 0) {
        std::cerr << "[import_debate_tool] Scenario '" << scenario.name() << "' has no steps." << std::endl;
        return 1;
    }

    std::cout << "[import_debate_tool] Importing scenario '" << scenario.name() << "' ("
               << scenario.steps_size() << " step(s))" << std::endl;
    std::cout << "                     debates: " << utils::getDatabasePath() << std::endl;
    std::cout << "                     users:   " << users_db_path << std::endl;

    Database usersDb(users_db_path);
    DebateModerator moderator(usersDb);
    scenario::Importer importer(moderator);
    const bool success = importer.Run(scenario);

    return success ? 0 : 1;
}
