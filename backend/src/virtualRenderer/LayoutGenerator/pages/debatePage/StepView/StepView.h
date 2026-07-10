#pragma once

#include "../../../../../../../src/gen/cpp/layout.pb.h"
#include "../../../../../../../src/gen/cpp/rendering_info.pb.h"
#include "../../../../../../../src/gen/cpp/collection.pb.h"
#include "../../../../../../../src/gen/cpp/user.pb.h"

class VRUserDatabase;

class StepView {
public:
	static ui::Page GenerateStepViewPage(
		const rendering_info::FullDebateViewInfo& fullDebateInfo,
		const debate::Collection& collectionProto,
		VRUserDatabase& userDb,
		const user::User& viewerUser
	);
};
