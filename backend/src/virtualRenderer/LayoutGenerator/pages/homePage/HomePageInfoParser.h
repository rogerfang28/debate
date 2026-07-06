#pragma once

#include "moderator_to_vr.pb.h"
#include "rendering_info.pb.h"

class HomePageInfoParser {
public:
    static rendering_info::HomePageRenderingInfo ParseFromResponse(const moderator_to_vr::ModeratorToVRMessage& responseMessage);
};
