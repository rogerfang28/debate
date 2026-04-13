#include "StepView.h"
#include "../../../ComponentGenerator.h"
#include <string>

ui::Page StepView::GenerateStepViewPage(
	const rendering_info::FullDebateViewInfo& fullDebateInfo,
	const debate::Collection& collectionProto
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

		std::string description;
		auto claimIt = collectionProto.claims_by_id().find(claimId);
		if (claimIt != collectionProto.claims_by_id().end()) {
			description = claimIt->second.description();
		}

		ui::Component stepCard = ComponentGenerator::createContainer(
			"stepCard_" + std::to_string(claimId),
			"flex flex-col gap-2",
			"bg-gray-800",
			"p-4",
			"",
			"border border-gray-700",
			"rounded-lg",
			""
		);

		ui::Component claimIdText = ComponentGenerator::createText(
			"stepClaimId_" + std::to_string(claimId),
			"Claim ID: " + std::to_string(claimId),
			"text-xs",
			"text-gray-400",
			"font-semibold",
			""
		);
		ui::Component stepNumberText = ComponentGenerator::createText(
			"stepNumber_" + std::to_string(claimId),
			"Step " + std::to_string(i + 1),
			"text-base",
			"text-blue-300",
			"font-semibold",
			""
		);
		ui::Component sentenceText = ComponentGenerator::createText(
			"stepSentence_" + std::to_string(claimId),
			sentence,
			"text-base",
			"text-white",
			"font-medium",
			""
		);

		ComponentGenerator::addChild(&stepCard, claimIdText);
		ComponentGenerator::addChild(&stepCard, stepNumberText);
		ComponentGenerator::addChild(&stepCard, sentenceText);

		if (!description.empty()) {
			ui::Component descriptionText = ComponentGenerator::createText(
				"stepDescription_" + std::to_string(claimId),
				description,
				"text-sm",
				"text-gray-300",
				"",
				""
			);
			ComponentGenerator::addChild(&stepCard, descriptionText);
		}

		ComponentGenerator::addChild(&stepsContainer, stepCard);
	}

	ComponentGenerator::addChild(&container, stepsContainer);

	ui::Component* root = page.add_components();
	root->CopyFrom(container);
	return page;
}
