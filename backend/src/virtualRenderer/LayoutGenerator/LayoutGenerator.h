#pragma once

#include <string>
#include "moderator_to_vr.pb.h"
#include "layout.pb.h"
#include "../../database/virtualrenderer/VRUserDatabase.h"

class LayoutGenerator {
public:
    // Generate layout based on ModeratorToVRMessage
    // signInError / signInUsername: a failed sign-in from the sign-in modal,
    // shown in the modal (which then stays open) with the name prefilled.
    static ui::Page generateLayout(const moderator_to_vr::ModeratorToVRMessage& msg, VRUserDatabase& userDb,
                                   const std::string& signInError = "",
                                   const std::string& signInUsername = "");
};