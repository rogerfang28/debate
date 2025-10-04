#include "pageGenerator.h"
#include "../../../src/gen/cpp/page.pb.h"
#include "../database/databaseCommunicator.h"

#include <google/protobuf/text_format.h>
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>

std::string generatePage(const std::string& user) {
    // -------- load pbtxt with relative path from this file's location --------
    // Current file is in: cppserver/src/virtualRenderer/
    // Target file is in: virtualRenderer/pages/schemas/
    // So we go: ../../../virtualRenderer/pages/schemas/testPage.pbtxt
    std::filesystem::path pbtxtPath = "../virtualRenderer/pages/schemas/testPage.pbtxt";
    pbtxtPath = std::filesystem::absolute(pbtxtPath);

    std::cerr << "[PageGen] looking for pbtxt at " << pbtxtPath.u8string() << "\n";

    std::ifstream input(pbtxtPath, std::ios::binary);
    if (!input) {
        std::cerr << "[PageGen][ERR] couldn't open " << pbtxtPath.u8string() << "\n";
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    ui::Page page;
    if (!google::protobuf::TextFormat::ParseFromString(buffer.str(), &page)) {
        std::cerr << "[PageGen][ERR] TextFormat parse failed\n";
        return {};
    }

    // -------- read debates from DB --------
    auto debates = readDebates(user);
    std::cerr << "[PageGen] readDebates(\"" << user << "\") returned " 
              << debates.size() << " items\n";
    for (const auto& d : debates) {
        std::cerr << "    id=" << d.id << " topic=\"" << d.topic << "\"\n";
    }

    // -------- find topicsList component --------
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
        std::cerr << "[PageGen] injecting " << debates.size() 
                  << " debate topics into topicsList\n";

        topicsList->clear_items(); // remove any placeholder items
        for (const auto& d : debates) {
            auto* item = topicsList->add_items();
            item->set_label(d.topic);
            item->set_value("topic:" + std::to_string(d.id));
        }
    }

    // -------- serialize page --------
    std::string page_bin;
    if (!page.SerializeToString(&page_bin)) {
        std::cerr << "[PageGen][ERR] failed to serialize page\n";
        return {};
    }

    std::cerr << "[PageGen] generated page with size=" << page_bin.size() << " bytes\n";
    return page_bin;
}
