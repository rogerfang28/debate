#pragma once

#include <string>
#include "../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../src/gen/cpp/layout.pb.h"
#include "../../database/virtualrenderer/VRUserDatabase.h"

class LayoutGenerator {
public:
    // Generate layout based on ModeratorToVRMessage
    static ui::Page generateLayout(const moderator_to_vr::ModeratorToVRMessage& msg, VRUserDatabase& userDb);
};