#include "LayoutGenerator.h"

#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "./pages/homePage/HomePageGenerator.h"
#include "./pages/debatePage/DebatePageGenerator.h"
#include "./pages/loginPage/LoginPageGenerator.h"
#include "./pages/errorPage/ErrorPageGenerator.h"
#include "../../utils/Log.h"

ui::Page LayoutGenerator::generateLayout(const moderator_to_vr::ModeratorToVRMessage& info, const std::string& user) {
    // ok so we need to decode the info and generate a page based on it
    // so we should check user engagement first
    switch (info.engagement().current_action()){
        case user_engagement::ACTION_NONE:
            // generate home page
            Log::debug("[LayoutGenerator] Generating Home Page for user: " + user);

            // get the debate list from info
            if (info.has_engagement() && info.engagement().has_none_info()) {
                return HomePageGenerator::GenerateHomePage(info.engagement().none_info().available_debates());
            } else {
                Log::debug("[LayoutGenerator] No debate list found, generating empty Home Page.");
                user_engagement::DebateList emptyList;
                return HomePageGenerator::GenerateHomePage(emptyList);
            }
        case user_engagement::ACTION_DEBATING:
            // generate debate page
            Log::debug("[LayoutGenerator] Generating Debate Page for user: " + user);
            if (true) {
                std::string currentClaimId = info.engagement().debating_info().current_claim().id();
                std::string debate_topic = info.engagement().debating_info().root_claim().sentence();
                std::string debate_claim_sentence = info.engagement().debating_info().current_claim().sentence();
                std::vector<std::pair<std::string,std::string>> childClaims;
                for (int i = 0; i < info.engagement().debating_info().children_claims_size(); i++) {
                    const user_engagement::ClaimInfo& claim = info.engagement().debating_info().children_claims(i);
                    childClaims.push_back({claim.id(), claim.sentence()});
                }
                Log::debug("[LayoutGenerator] Debate Topic: " + debate_topic);
                Log::debug("[LayoutGenerator] Current Claim: " + debate_claim_sentence);
                Log::debug("[LayoutGenerator] Number of Child Claims: " + std::to_string(childClaims.size()));
                return DebatePageGenerator::GenerateDebatePage(debate_topic, debate_claim_sentence, childClaims);
            } else {
                Log::debug("[LayoutGenerator] No debate info found, generating Home Page instead.");
                user_engagement::DebateList emptyList;
                return HomePageGenerator::GenerateHomePage(emptyList);
            }
        default:
            Log::debug("[LayoutGenerator] Unknown engagement action, generating error not found page with a go home button.");
            Log::debug("[LayoutGenerator] engagement action: " + std::to_string(info.engagement().current_action()));
            return ErrorPageGenerator::GenerateErrorPage();
    }
}
