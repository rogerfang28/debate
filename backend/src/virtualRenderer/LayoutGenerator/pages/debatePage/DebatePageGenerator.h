#pragma once
#include "../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../src/gen/cpp/user.pb.h"

class DebatePageGenerator {
public:
    static ui::Page GenerateDebatePage(user::User user);
    static ui::Component GenerateDebatePageMainLayout();
    static ui::Component FillChildClaims(user::User user, ui::Component mainLayout);
    static ui::Component FillChallenges(user::User user, ui::Component mainLayout);
    static ui::Component FillCurrentClaimSection(user::User user, ui::Component mainLayout);
    static ui::Component AddAppropriateButtons(user::User user, ui::Component mainLayout);
    static ui::Component AddAppropriateOverlays(user::User user, ui::Component mainLayout);
};