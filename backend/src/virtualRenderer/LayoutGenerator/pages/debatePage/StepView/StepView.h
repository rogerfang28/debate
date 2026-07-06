#pragma once

#include "layout.pb.h"
#include "rendering_info.pb.h"
#include "collection.pb.h"

class VRUserDatabase;

class StepView {
public:
	static ui::Page GenerateStepViewPage(
		const rendering_info::FullDebateViewInfo& fullDebateInfo,
		const debate::Collection& collectionProto,
		VRUserDatabase& userDb
	);
};
