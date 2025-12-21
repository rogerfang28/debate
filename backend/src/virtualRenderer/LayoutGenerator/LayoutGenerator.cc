#include "LayoutGenerator.h"

// #include "./pageGenerator.h"
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "./pages/homePage/HomePageGenerator.h"
#include "./pages/debatePage/DebatePageGenerator.h"
#include "./pages/loginPage/LoginPageGenerator.h"
#include "./pages/errorPage/ErrorPageGenerator.h"

// #include "../utils/pathUtils.h"

ui::Page LayoutGenerator::generateLayout(const moderator_to_vr::ModeratorToVRMessage& info, const std::string& user) {
    // ok so we need to decode the info and generate a page based on it
    // so we should check user engagement first
    switch (info.engagement().current_action()){
        case user_engagement::ACTION_NONE:
            // generate home page
            std::cout << "[LayoutGenerator] Generating Home Page for user: " << user << "\n";

            // get the debate list from info
            if (info.has_users_debates()) {
                return HomePageGenerator::GenerateHomePage(info.users_debates());
            } else {
                debate::DebateList emptyList;
                return HomePageGenerator::GenerateHomePage(emptyList);
            }
        case user_engagement::ACTION_DEBATING:
            // generate debate page
            std::cout << "[LayoutGenerator] Generating Debate Page for user: " << user << "\n";
            if (info.has_debate()) {
                const auto& debate = info.debate();
                // find the claim that the user is on
                std::string currentClaimId = info.engagement().debating_info().current_claim_id();
                // go through the debate to find the claim
                debate::Claim claim;
                std::cout << "[LayoutGenerator] Searching for current claim ID: " << currentClaimId << "\n";
                for (const auto& c : debate.claims()) {
                    if (c.id() == currentClaimId) {
                        std::cout << "[LayoutGenerator] Current Claim found: " << c.sentence() << "\n";
                        claim = c;
                        break;
                    }
                }
                // now i have the claim
                // i need to find its children
                std::vector<debate::Claim> childClaims;
                for (const auto& childId: claim.children_ids()) {
                    // find the child claim by id
                    for (const auto& c : debate.claims()) {
                        if (c.id() == childId) {
                            childClaims.push_back(c);
                            break;
                        }
                    }
                }
                // test hardcode a child claim
                // debate::Claim testChild;
                // testChild.set_id("2");
                // testChild.set_sentence("This is a test child claim.");
                // testChild.set_description("This is the description of the test child claim.");
                // childClaims.push_back(testChild);
                // it worked

                return DebatePageGenerator::GenerateDebatePage(debate.topic(), claim.sentence(), childClaims);
            } else {
                std::cout << "[LayoutGenerator] No debate info found, generating Home Page instead.\n";
                debate::DebateList emptyList;
                return HomePageGenerator::GenerateHomePage(emptyList);
            }
        default:
            std::cout << "[LayoutGenerator] Unknown engagement action, generating error not found page with a go home button.\n";
            std::cout << "[LayoutGenerator] engagement action: " << info.engagement().current_action() << "\n";
            return ErrorPageGenerator::GenerateErrorPage();
    }
}
