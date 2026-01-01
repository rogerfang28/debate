#pragma once
#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user_engagement.pb.h"

class DebatePageGenerator {
public:
    static ui::Page GenerateDebatePage(user_engagement::UserEngagement engagement);
    static ui::Component GenerateDebatePageMainLayout();
    static ui::Component FillChildClaims(user_engagement::UserEngagement engagement, ui::Component mainLayout);
    static ui::Component FillChallenges(user_engagement::UserEngagement engagement, ui::Component mainLayout);
    static ui::Component FillCurrentClaimSection(user_engagement::UserEngagement engagement, ui::Component mainLayout);
    static ui::Component AddAppropriateButtons(user_engagement::UserEngagement engagement, ui::Component mainLayout);
    static ui::Component AddAppropriateOverlays(user_engagement::UserEngagement engagement, ui::Component mainLayout);
};