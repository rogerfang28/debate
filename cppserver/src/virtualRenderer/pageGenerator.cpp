#include "pageGenerator.h"
#include "../../../src/gen/cpp/page.pb.h"
#include "../database/databaseCommunicator.h"

#include <google/protobuf/text_format.h>
#include <fstream>
#include <sstream>
#include <iostream>

std::string generatePage(const std::string& user,
                         const std::filesystem::path& exeDir) {
    // For now, just load the same pbtxt you had before
    auto pbtxtPath = exeDir / ".." / "virtualRenderer" / "pages" / "schemas" / "testPage.pbtxt";
    pbtxtPath = std::filesystem::weakly_canonical(pbtxtPath);

    std::cerr << "generatePage(): looking for pbtxt at " << pbtxtPath.u8string() << "\n";

    std::ifstream input(pbtxtPath, std::ios::binary);
    if (!input) {
        std::cerr << "ERR: couldn't open " << pbtxtPath.u8string() << "\n";
        return {};
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    ui::Page page;
    if (!google::protobuf::TextFormat::ParseFromString(buffer.str(), &page)) {
        std::cerr << "ERR: TextFormat parse failed\n";
        return {};
    }

    std::string page_bin;
    if (!page.SerializeToString(&page_bin)) {
        std::cerr << "ERR: failed to serialize page\n";
        return {};
    }

    // now add the submitted topics
    // readDebates();

    return page_bin;
}
