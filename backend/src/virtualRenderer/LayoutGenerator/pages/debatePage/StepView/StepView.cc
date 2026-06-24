#include "StepView.h"
#include "../../../ComponentGenerator.h"
#include "../FullDebateView/FullDebatePageGenerator.h"
#include "../../../../utils/UserNameResolver.h"
#include <string>

ui::Page StepView::GenerateStepViewPage(
	const rendering_info::FullDebateViewInfo& fullDebateInfo,
	const debate::Collection& collectionProto,
	VRUserDatabase& userDb
) {
	ui::Page page;
	page.set_page_id("debate");
	page.set_title("commit: " GIT_COMMIT_HASH);

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
		"min-h-screen flex flex-col items-stretch",
		"bg-gray-900",
		"p-6",
		"",
		"",
		"",
		"gap-4"
	);

	// Intentionally no top bar action button in Step View.

	ui::Component contentRow = ComponentGenerator::createContainer(
		"stepViewContentRow",
		"w-full",
		"",
		"",
		"",
		"",
		"",
		""
	);
    (*contentRow.mutable_css())["display"] = "flex";
    (*contentRow.mutable_css())["flex-direction"] = "row";
    (*contentRow.mutable_css())["flex-wrap"] = "nowrap";
    (*contentRow.mutable_css())["align-items"] = "flex-start";
    (*contentRow.mutable_css())["gap"] = "1rem";

	ui::Component leftColumn = ComponentGenerator::createContainer(
		"stepViewLeftColumn",
		"",
		"",
		"",
		"",
		"",
		"",
		""
	);
    (*leftColumn.mutable_css())["display"] = "flex";
    (*leftColumn.mutable_css())["flex-direction"] = "column";
    (*leftColumn.mutable_css())["gap"] = "1rem";
	(*leftColumn.mutable_css())["flex"] = "1 1 auto";
	(*leftColumn.mutable_css())["min-width"] = "0";

	ui::Component rightColumn = ComponentGenerator::createContainer(
		"stepViewRightColumn",
		"",
		"",
		"",
		"",
		"",
		"",
		""
	);
	(*rightColumn.mutable_css())["display"] = "flex";
	(*rightColumn.mutable_css())["flex-direction"] = "row";
	(*rightColumn.mutable_css())["flex"] = "0 0 45%";
	(*rightColumn.mutable_css())["min-width"] = "400px";

	ui::Component guideBox = ComponentGenerator::createContainer(
		"stepViewGuideBox",
		"w-full",
		"bg-blue-900/30",
		"p-4",
		"",
		"border border-blue-700",
		"rounded-lg",
		""
	);
	(*guideBox.mutable_css())["max-width"] = "100%";
	ui::Component guideTitle = ComponentGenerator::createText(
		"stepViewGuideTitle",
		"Guide",
		"text-sm",
		"text-blue-200",
		"font-semibold uppercase tracking-wide",
		"mb-2"
	);
	ui::Component guideParagraph = ComponentGenerator::createText(
		"stepViewGuideParagraph",
		"This example shows how an anti-vax influencer was challenged. Two mock users, an influencer and a challenger, debated for two rounds using this tool. You are viewing the aftermath.\n\nThe tree structure visualizes how every statement is logically connected, including the challenging statements against each other.\n\nAI segmented this debate into steps and generated a summary (currently mocked) for your convenience. You may click on each step to jump to its vital statement in the debate.\n\n(The power of this tool resides in that all logic within a narrative is up for challenge. And challenges can not be dodged.)\n\nIt’s work in progress. Efforts are needed to make it fully functioning and easy to understand for non-tech users.",
		"text-base",
		"text-blue-100",
		"",
		"leading-snug"
	);
	(*guideParagraph.mutable_css())["white-space"] = "pre-line";
	(*guideParagraph.mutable_css())["overflow-wrap"] = "break-word";
	(*guideParagraph.mutable_css())["text-align"] = "left";
	(*guideParagraph.mutable_css())["font-style"] = "italic";
	ComponentGenerator::addChild(&guideBox, guideTitle);
	ComponentGenerator::addChild(&guideBox, guideParagraph);

	ui::Component stepsContainer = ComponentGenerator::createContainer(
		"stepCardsContainer",
		"w-full grid grid-cols-1 gap-4",
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
			sentence,
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

	ui::Component influencerNoteCard = ComponentGenerator::createContainer(
		"stepCard_influencerConcedeNote",
		"flex items-start justify-between gap-3",
		"bg-gray-800",
		"p-4",
		"",
		"border border-gray-700",
		"rounded-lg",
		""
	);
	ui::Component influencerNoteText = ComponentGenerator::createText(
		"stepViewInfluencerConcedeNote",
		"Now influencer has to concede (to be implemented)",
		"text-base",
		"text-white",
		"font-medium",
		"flex-1"
	);
	ComponentGenerator::addChild(&influencerNoteCard, influencerNoteText);
	ComponentGenerator::addChild(&stepsContainer, influencerNoteCard);

	ComponentGenerator::addChild(&leftColumn, guideBox);
	ComponentGenerator::addChild(&leftColumn, stepsContainer);

	const int mapFocusClaimId = rootClaimId > 0
		? rootClaimId
		: (fullDebateInfo.steps_size() > 0 ? fullDebateInfo.steps(0).claim_id() : 0);
	const float mapScale = 0.82f;
	ui::Component mapSection = FullDebatePageGenerator::GenerateMapSection(fullDebateInfo, mapFocusClaimId, mapScale);
	for (int childIndex = mapSection.children_size() - 1; childIndex >= 0; --childIndex) {
		if (mapSection.children(childIndex).id() == "mapTitle") {
			mapSection.mutable_children()->DeleteSubrange(childIndex, 1);
		}
	}
	(*mapSection.mutable_css())["margin-top"] = "0";
	(*mapSection.mutable_css())["max-width"] = "760px";
	(*mapSection.mutable_css())["width"] = "100%";
	ComponentGenerator::addChild(&rightColumn, mapSection);

	ComponentGenerator::addChild(&contentRow, leftColumn);
	ComponentGenerator::addChild(&contentRow, rightColumn);
	ComponentGenerator::addChild(&container, contentRow);

	ui::Component* root = page.add_components();
	root->CopyFrom(container);
	return page;
}
