#pragma once

#include "user.pb.h"
#include "collection.pb.h"
#include "rendering_info.pb.h"

class FullDebatePageInfoParser {
public:
	static rendering_info::DebatePageRenderingInfo ParseFromUser(user::User userProto, const debate::Collection& collectionProto);
	static rendering_info::FullDebateViewInfo ParseFullDebateViewInfo(const debate::Collection& collectionProto, int viewer_user_id, const std::string& viewer_username, int viewer_current_claim_id = 0);
};
