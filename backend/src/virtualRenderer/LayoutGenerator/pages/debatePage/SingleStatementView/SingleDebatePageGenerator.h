#pragma once
#include "layout.pb.h"
#include "debate.pb.h"
#include "rendering_info.pb.h"
#include "user.pb.h"

class DebatePageGenerator {
public:
    static ui::Page GenerateSingleDebatePage(const rendering_info::DebatePageRenderingInfo& info, const user::User& userProto);
    static ui::Component GenerateSingleClaimLayout();
    static ui::Component FillChildClaims(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
    static ui::Component FillChallenges(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
    static ui::Component FillCurrentClaimSection(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
    static ui::Component AddAppropriateButtons(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
    static ui::Component AddAppropriateOverlays(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout);
};