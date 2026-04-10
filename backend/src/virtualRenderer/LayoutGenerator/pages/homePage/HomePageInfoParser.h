#pragma once

#include "../../../../../../src/gen/cpp/moderator_to_vr.pb.h"
#include "../../../../../../src/gen/cpp/rendering_info.pb.h"

class HomePageInfoParser {
public:
    static rendering_info::HomePageRenderingInfo ParseFromResponse(const moderator_to_vr::ModeratorToVRMessage& responseMessage);
};
