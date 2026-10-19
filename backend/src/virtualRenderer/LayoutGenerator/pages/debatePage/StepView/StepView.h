#pragma once

#include "layout.pb.h"
#include "rendering_info.pb.h"
#include "collection.pb.h"
#include "user.pb.h"

class VRUserDatabase;

class StepView {
public:
	static ui::Page GenerateStepViewPage(
		const rendering_info::FullDebateViewInfo& fullDebateInfo,
		const debate::Collection& collectionProto,
		VRUserDatabase& userDb,
		const user::User& viewerUser
	);

	// The docked AI chat. Shared with the Brief tab, which is the same
	// debate seen another way and should answer the same questions.
	static ui::Component BuildChatPanel(const user::User& viewerUser, int selectedClaimId);

	// The chat panel as a page's chat region (data-region="chat").
	static ui::Component BuildChatColumn(const user::User& viewerUser, int selectedClaimId);

	// The Debate | Brief tabs at the top of both pages. One is lit.
	static ui::Component BuildDebateTabs(bool briefActive);

	// One tab button in the theme's look; the lit one is the current view.
	static ui::Component BuildTab(const std::string& id, const std::string& label, bool lit);
};
