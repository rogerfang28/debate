#pragma once

#include "user.pb.h"
#include "collection.pb.h"
#include "rendering_info.pb.h"

class DebatePageInfoParser {
public:
	static rendering_info::DebatePageRenderingInfo ParseFromUser(user::User userProto, const debate::Collection& collectionProto);
};
