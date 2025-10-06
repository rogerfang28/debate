#include "pageGenerator.h"
#include "../../../src/gen/cpp/page.pb.h"
#include "../debate/DebateDatabaseHandler.h"   // use this instead of databaseCommunicator

#include <google/protobuf/text_format.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

std::string generatePage(const std::string& user) {
    // -------- Load page schema --------
    std::filesystem::path pbtxtPath = "../virtualRenderer/pages/schemas/testPage.pbtxt";
    pbtxtPath = std::filesystem::absolute(pbtxtPath);

    std::cerr << "[PageGen] Looking for pbtxt at " << pbtxtPath.u8string() << "\n";

    std::ifstream input(pbtxtPath, std::ios::binary);
    if (!input) {
        std::cerr << "[PageGen][ERR] Couldn't open " << pbtxtPath.u8string() << "\n";
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    ui::Page page;
    if (!google::protobuf::TextFormat::ParseFromString(buffer.str(), &page)) {
        std::cerr << "[PageGen][ERR] TextFormat parse failed\n";
        return {};
    }

    // -------- Fetch debates for user --------
    DebateDatabaseHandler dbHandler("debates.sqlite3");
    auto debates = dbHandler.getDebates(user);

    std::cerr << "[PageGen] getDebates(\"" << user << "\") returned "
              << debates.size() << " items\n";
    for (const auto& d : debates) {
        std::cerr << "    id=" << d.at("ID") << " topic=\"" << d.at("TOPIC") << "\"\n";
    }

    // -------- Find topicsList component --------
    ui::Component* topicsList = nullptr;
    if (page.components_size() > 0) {
        auto* root = page.mutable_components(0);
        for (auto& comp : *root->mutable_children()) {
            if (comp.id() == "topicsCard") {
                for (auto& child : *comp.mutable_children()) {
                    if (child.id() == "topicsList") {
                        topicsList = &child;
                        break;
                    }
                }
            }
        }
    }

    if (!topicsList) {
        std::cerr << "[PageGen][WARN] topicsList component not found in page schema\n";
    } else {
        std::cerr << "[PageGen] Injecting " << debates.size()
                  << " debate topics into topicsList\n";

        topicsList->clear_items();
        for (const auto& row : debates) {
            auto* item = topicsList->add_items();
            item->set_label(row.at("TOPIC"));
            item->set_value("topic:" + row.at("ID"));
        }
    }

    // -------- Serialize final page --------
    std::string page_bin;
    if (!page.SerializeToString(&page_bin)) {
        std::cerr << "[PageGen][ERR] Failed to serialize page\n";
        return {};
    }

    std::cerr << "[PageGen] Generated page with size=" << page_bin.size() << " bytes\n";
    return page_bin;
}
