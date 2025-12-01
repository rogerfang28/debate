#include "LayoutGenerator.h"

// #include "./pageGenerator.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../pages/generators/homePage/HomePageGenerator.h"
#include "../pages/generators/debatePage/DebateClaimPageGenerator.h"
#include "../pages/generators/loginPage/LoginPageGenerator.h"

// #include "../utils/pathUtils.h"

ui::Page LayoutGenerator::generateLayout(const moderator_to_vr::ModeratorToVRMessage& info, const std::string& user) {
    // for now call pagegenerator
    // std::string page_bin = generatePage(user);
    std::string page_bin = HomePageGenerator::GenerateHomePage().SerializeAsString();
    page_bin = DebateClaimPageGenerator::GenerateDebatePage("e","f").SerializeAsString();
    ui::Page page;
    page.ParseFromString(page_bin);
    return page;
}
