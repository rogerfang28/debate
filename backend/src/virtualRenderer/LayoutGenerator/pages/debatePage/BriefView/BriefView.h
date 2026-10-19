#pragma once

#include "layout.pb.h"
#include "rendering_info.pb.h"
#include "collection.pb.h"
#include "user.pb.h"

class VRUserDatabase;

// The Brief tab: the debate for someone who has to catch up.
//
// Read-only. Verdict, the story, who stands where, each disagreement as the
// back-and-forth it was with a dot saying whether someone still owes a
// reply, what nobody disputes, what the viewer owes, how it got here, and
// the sources. Every line can jump to its claim on the Debate tab.
//
// The structure is computed from the collection on every build (see
// DebateBrief). The words the model wrote ride on the viewer's record; when
// the structure they describe is no longer this one, the page says so and
// falls back to the claims' own sentences.
class BriefView {
public:
    static ui::Page GenerateBriefPage(
        const rendering_info::FullDebateViewInfo& fullDebateInfo,
        const debate::Collection& collectionProto,
        VRUserDatabase& userDb,
        const user::User& viewerUser
    );
};
