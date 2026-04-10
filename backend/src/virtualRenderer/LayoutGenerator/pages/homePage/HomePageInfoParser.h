#pragma once

#include "../../../../../../src/gen/cpp/user.pb.h"
#include "../../../../../../src/gen/cpp/rendering_info.pb.h"

class HomePageInfoParser {
public:
    static rendering_info::HomePageRenderingInfo ParseFromUser(const user::User& userProto);
};
