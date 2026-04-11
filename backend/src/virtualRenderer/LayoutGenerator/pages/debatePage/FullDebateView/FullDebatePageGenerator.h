#pragma once
#include "../../../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../../../src/gen/cpp/debate.pb.h"
#include "../../../../../../../src/gen/cpp/rendering_info.pb.h"
#include "../../../../../../../src/gen/cpp/user.pb.h"

class DebatePageGenerator {
public:
    static ui::Page GenerateDebatePage(const rendering_info::DebatePageRenderingInfo& info, const user::User& userProto);
    static ui::Component GenerateSingleClaimLayout();
    static ui::Component FillChildClaims(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
    static ui::Component FillChallenges(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
    static ui::Component FillCurrentClaimSection(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
    static ui::Component AddAppropriateButtons(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
    static ui::Component AddAppropriateOverlays(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
};