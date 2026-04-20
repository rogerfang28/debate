#include "StepView.h"
#include "../../../ComponentGenerator.h"
#include "../../../../utils/UserNameResolver.h"
#include <string>

ui::Page StepView::GenerateStepViewPage(
	const rendering_info::FullDebateViewInfo& fullDebateInfo,
	const debate::Collection& collectionProto,
	VRUserDatabase& userDb
) {
	ui::Page page;
	page.set_page_id("debate");
	page.set_title("Debate Overview");

	int rootClaimId = -1;
	std::string rootSentence = "Debate overview";
	if (fullDebateInfo.has_full_debate_tree()) {
		rootClaimId = fullDebateInfo.full_debate_tree().root_claim_id();
	}
	if (rootClaimId > 0) {
		auto rootIt = collectionProto.claims_by_id().find(rootClaimId);
		if (rootIt != collectionProto.claims_by_id().end() && !rootIt->second.sentence().empty()) {
			rootSentence = rootIt->second.sentence();
		}
	}

	ui::Component container = ComponentGenerator::createContainer(
		"stepViewMain",
		"min-h-screen flex flex-col items-center",
		"bg-gray-900",
		"p-6",
		"",
		"",
		"",
		"gap-4"
	);

	ui::Component topBar = ComponentGenerator::createContainer(
		"stepViewTopBar",
		"w-full max-w-5xl flex items-start justify-between gap-4",
		"",
		"",
		"",
		"",
		"",
		""
	);

	ui::Component titleBlock = ComponentGenerator::createContainer(
		"stepViewTitleBlock",
		"flex flex-col gap-1",
		"",
		"",
		"",
		"",
		"",
		""
	);
	ui::Component titleLabel = ComponentGenerator::createText(
		"stepViewTitleLabel",
		"Debate Root Claim",
		"text-sm",
		"text-gray-300",
		"font-semibold",
		"uppercase tracking-wide"
	);
	ui::Component titleText = ComponentGenerator::createText(
		"stepViewTitleText",
		rootSentence,
		"text-xl",
		"text-white",
		"font-bold",
		""
	);
	ComponentGenerator::addChild(&titleBlock, titleLabel);
	ComponentGenerator::addChild(&titleBlock, titleText);
	ComponentGenerator::addChild(&topBar, titleBlock);

	ui::Component testButton = ComponentGenerator::createButton(
		"stepViewTestButton",
		"Back to Full Debate",
		"",
		"bg-blue-600",
		"hover:bg-blue-700",
		"text-white",
		"px-4 py-2",
		"rounded",
		"text-sm"
	);
	ComponentGenerator::addChild(&topBar, testButton);
	ComponentGenerator::addChild(&container, topBar);

	ui::Component stepsContainer = ComponentGenerator::createContainer(
		"stepCardsContainer",
		"w-full max-w-5xl grid grid-cols-1 gap-4",
		"",
		"",
		"",
		"",
		"",
		""
	);

	for (int i = 0; i < fullDebateInfo.steps_size(); ++i) {
		const rendering_info::Steps& step = fullDebateInfo.steps(i);
		const int claimId = step.claim_id();
		const std::string sentence = step.summary();

		int creatorId = 0;
		std::string creatorUsername;
		auto claimIt = collectionProto.claims_by_id().find(claimId);
		if (claimIt != collectionProto.claims_by_id().end()) {
			creatorId = claimIt->second.creator_id();
			creatorUsername = UserNameResolver::ResolveUsername(creatorId, userDb);
		}

		ui::Component stepCard = ComponentGenerator::createContainer(
			"stepCard_" + std::to_string(claimId),
			"flex items-start justify-between gap-3",
			"bg-gray-800",
			"p-4",
			"",
			"border border-gray-700",
			"rounded-lg",
			""
		);

		std::string creatorDisplay = creatorUsername.empty()
			? ("user " + std::to_string(creatorId))
			: creatorUsername;
		ui::Component sentenceText = ComponentGenerator::createText(
			"stepSentence_" + std::to_string(claimId),
			creatorDisplay + " says: " + sentence,
			"text-base",
			"text-white",
			"font-medium",
			"flex-1"
		);
		ui::Component goToStepButton = ComponentGenerator::createButton(
			"fullDebateStepButton_" + std::to_string(claimId),
			"Go to Claim",
			"",
			"bg-gray-600",
			"hover:bg-gray-700",
			"text-white",
			"px-2 py-1",
			"rounded",
			"text-xs w-fit shrink-0 self-start"
		);

		ComponentGenerator::addChild(&stepCard, sentenceText);
		ComponentGenerator::addChild(&stepCard, goToStepButton);

		ComponentGenerator::addChild(&stepsContainer, stepCard);
	}

	ComponentGenerator::addChild(&container, stepsContainer);

	ui::Component* root = page.add_components();
	root->CopyFrom(container);
	return page;
}
