#pragma once

#include "../../../../../../../src/gen/cpp/user.pb.h"
#include "../../../../../../../src/gen/cpp/collection.pb.h"
#include "../../../../../../../src/gen/cpp/rendering_info.pb.h"

class DebatePageInfoParser {
public:
	static rendering_info::DebatePageRenderingInfo ParseFromUser(user::User userProto, const debate::Collection& collectionProto);
};
