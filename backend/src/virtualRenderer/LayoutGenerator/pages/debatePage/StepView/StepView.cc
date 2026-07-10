#include "StepView.h"
#include "../../../ComponentGenerator.h"
#include "../FullDebateView/FullDebatePageGenerator.h"
#include "../FullDebateView/FullDebatePageInfoParser.h"
#include "../../../../utils/UserNameResolver.h"
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Local copy of FullDebatePageGenerator.cc's ClaimStatusToLabel -- that one
// isn't declared in a header, so it's not reusable across translation units.
static std::string StepViewClaimStatusToLabel(debate::ClaimStatus status) {
    switch (status) {
        case debate::ClaimStatus::UNDETERMINED:
            return "Undetermined";
        case debate::ClaimStatus::TRUE_CLAIM:
            return "True";
        case debate::ClaimStatus::FALSE_CLAIM:
            return "False";
        default:
            return "Unknown";
    }
}

// Builds the "Selected Claim" info panel: sentence, status, creator,
// description, and contextual action buttons (Add Child + Delete for the
// viewer's own claims, Challenge for others').
static ui::Component BuildSelectedClaimPanel(
    int claimId,
    const debate::Collection& collectionProto,
    VRUserDatabase& userDb,
    int viewerUserId
) {
    ui::Component panel = ComponentGenerator::createContainer(
        "selectedClaimPanel",
        "w-full",
        "bg-gray-800",
        "p-4",
        "",
        "border border-gray-700",
        "rounded-lg",
        ""
    );
    (*panel.mutable_css())["display"] = "flex";
    (*panel.mutable_css())["flex-direction"] = "column";
    (*panel.mutable_css())["gap"] = "0.5rem";

    auto claimIt = collectionProto.claims_by_id().find(claimId);
    if (claimId <= 0 || claimIt == collectionProto.claims_by_id().end()) {
        ui::Component emptyText = ComponentGenerator::createText(
            "selectedClaimEmpty",
            "No claim selected.",
            "text-sm",
            "text-gray-400",
            "",
            ""
        );
        ComponentGenerator::addChild(&panel, emptyText);
        return panel;
    }
    const debate::Claim& claim = claimIt->second;

    auto addField = [&](const std::string& idSuffix, const std::string& label, const std::string& value, bool italic) {
        ui::Component row = ComponentGenerator::createContainer(
            "selectedClaim" + idSuffix + "Row",
            "",
            "",
            "",
            "",
            "",
            "",
            ""
        );
        (*row.mutable_css())["display"] = "flex";
        (*row.mutable_css())["flex-direction"] = "column";
        (*row.mutable_css())["gap"] = "0.15rem";

        ui::Component labelText = ComponentGenerator::createText(
            "selectedClaim" + idSuffix + "Label",
            label,
            "text-xs",
            "text-gray-500",
            "font-semibold uppercase tracking-wide",
            ""
        );
        ComponentGenerator::addChild(&row, labelText);

        ui::Component valueText = ComponentGenerator::createText(
            "selectedClaim" + idSuffix + "Value",
            value,
            "text-sm",
            "text-white",
            "",
            italic ? "italic" : ""
        );
        (*valueText.mutable_css())["overflow-wrap"] = "break-word";
        ComponentGenerator::addChild(&row, valueText);

        ComponentGenerator::addChild(&panel, row);
    };

    addField("Sentence", "Selected Claim:", claim.sentence(), false);
    addField("Status", "Status:", StepViewClaimStatusToLabel(claim.status()), false);
    std::string creatorUsername = userDb.getUsername(claim.creator_id());
    if (creatorUsername.empty()) {
        creatorUsername = "User " + std::to_string(claim.creator_id());
    }
    addField("Creator", "Created by:", creatorUsername, false);
    addField(
        "Description",
        "Description:",
        claim.description().empty() ? "(no description)" : claim.description(),
        claim.description().empty()
    );

    ui::Component buttonRow = ComponentGenerator::createContainer(
        "selectedClaimButtonRow",
        "flex gap-2",
        "",
        "",
        "",
        "",
        "",
        ""
    );
    (*buttonRow.mutable_css())["margin-top"] = "0.5rem";

    const bool isOwnClaim = viewerUserId > 0 && claim.creator_id() == viewerUserId;
    if (isOwnClaim) {
        ui::Component addChildButton = ComponentGenerator::createButton(
            "addChildClaimButton",
            "Add Child",
            "",
            "bg-blue-600",
            "hover:bg-blue-700",
            "text-white",
            "px-3 py-1.5",
            "rounded",
            "text-sm"
        );
        ComponentGenerator::addChild(&buttonRow, addChildButton);

        ui::Component deleteButton = ComponentGenerator::createButton(
            "deleteStatementButton",
            "Delete",
            "",
            "bg-red-600",
            "hover:bg-red-700",
            "text-white",
            "px-3 py-1.5",
            "rounded",
            "text-sm"
        );
        ComponentGenerator::addChild(&buttonRow, deleteButton);
    } else {
        ui::Component challengeButton = ComponentGenerator::createButton(
            "challengeClaimButton",
            "Challenge",
            "",
            "bg-orange-600",
            "hover:bg-orange-700",
            "text-white",
            "px-3 py-1.5",
            "rounded",
            "text-sm"
        );
        ComponentGenerator::addChild(&buttonRow, challengeButton);
    }
    ComponentGenerator::addChild(&panel, buttonRow);

    return panel;
}

// Helper: Build a nested tree of CONTAINER components from the debate tree.
// Each node becomes a CONTAINER with data-tree-node attribute.
// Returns the root component for the subtree rooted at `claimId`.
static ui::Component BuildTreeNode(
    int claimId,
    const rendering_info::FullDebateViewInfo& fullDebateInfo,
    const debate::Collection& collectionProto,
    const std::string& idPrefix,
    std::unordered_set<int>& visited,
    int currentClaimId
) {
    ui::Component node;
    node.set_id(idPrefix + "_node_" + std::to_string(claimId));
    node.set_type(ui::ComponentType::CONTAINER);
    ComponentGenerator::addAttribute(&node, "data-tree-node", "true");
    ComponentGenerator::addAttribute(&node, "data-node-id", std::to_string(claimId));
    if (currentClaimId > 0 && claimId == currentClaimId) {
        ComponentGenerator::addAttribute(&node, "data-tree-current", "true");
    }

    // Get sentence for this claim.
    std::string sentence;
    auto claimIt = collectionProto.claims_by_id().find(claimId);
    if (claimIt != collectionProto.claims_by_id().end()) {
        sentence = claimIt->second.sentence();
    }

    // Find children in the full_debate_tree: PARENT_CHILD children from the
    // convenience adjacency list, plus CHALLENGE targets from the tree's
    // link list (child_claim_ids only ever carries PARENT_CHILD edges).
    std::vector<std::pair<int, bool>> children; // (claimId, isChallenge)
    if (fullDebateInfo.has_full_debate_tree()) {
        const auto& tree = fullDebateInfo.full_debate_tree();
        for (const auto& treeNode : tree.nodes()) {
            if (treeNode.claim_id() == claimId) {
                for (int cid : treeNode.child_claim_ids()) {
                    children.emplace_back(cid, false);
                }
                break;
            }
        }
        for (const auto& link : tree.links()) {
            if (link.is_challenge() && link.from_claim_id() == claimId) {
                children.emplace_back(link.to_claim_id(), true);
            }
        }
    }

    // Text label for this node.
    ui::Component label = ComponentGenerator::createText(
        idPrefix + "_label_" + std::to_string(claimId),
        sentence,
        "text-sm",
        children.empty() ? "text-gray-300" : "text-blue-300",
        children.empty() ? "" : "font-medium",
        "cursor-pointer ms-1"
    );
    ComponentGenerator::addAttribute(&label, "data-tree-label", "true");
    ComponentGenerator::addChild(&node, label);

    // Recursively build children.
    if (!children.empty()) {
        ui::Component childrenContainer;
        childrenContainer.set_id(idPrefix + "_children_" + std::to_string(claimId));
        childrenContainer.set_type(ui::ComponentType::CONTAINER);
        ComponentGenerator::addAttribute(&childrenContainer, "data-tree-children", "true");
        (*childrenContainer.mutable_css())["display"] = "flex";
        (*childrenContainer.mutable_css())["flex-direction"] = "column";
        (*childrenContainer.mutable_css())["margin-left"] = "1.25rem";
        (*childrenContainer.mutable_css())["border-left"] = "1px solid #4b5563";
        (*childrenContainer.mutable_css())["padding-left"] = "0.75rem";
        (*childrenContainer.mutable_css())["gap"] = "0.25rem";

        for (const auto& [childId, isChallenge] : children) {
            if (visited.count(childId)) continue;
            visited.insert(childId);
            ui::Component childNode = BuildTreeNode(
                childId, fullDebateInfo, collectionProto,
                idPrefix, visited, currentClaimId
            );
            if (isChallenge) {
                ComponentGenerator::addAttribute(&childNode, "data-tree-challenge", "true");
            }
            ComponentGenerator::addChild(&childrenContainer, childNode);
        }
        ComponentGenerator::addChild(&node, childrenContainer);
    }

    return node;
}

ui::Page StepView::GenerateStepViewPage(
	const rendering_info::FullDebateViewInfo& fullDebateInfo,
	const debate::Collection& collectionProto,
	VRUserDatabase& userDb,
	const user::User& viewerUser
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

	// The viewer's actually-selected claim, if it exists in this debate;
	// falls back to the root claim so the map/tree still highlight something.
	int currentClaimId = fullDebateInfo.viewer_current_claim_id();
	if (currentClaimId <= 0 || collectionProto.claims_by_id().find(currentClaimId) == collectionProto.claims_by_id().end()) {
		currentClaimId = rootClaimId;
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

	ui::Component topBar = ComponentGenerator::createContainer(
		"stepViewTopBar",
		"w-full",
		"",
		"",
		"",
		"",
		"",
		""
	);
	(*topBar.mutable_css())["display"] = "flex";
	(*topBar.mutable_css())["flex-direction"] = "row";
	(*topBar.mutable_css())["justify-content"] = "flex-end";
	(*topBar.mutable_css())["align-items"] = "flex-start";
	ui::Component goHomeButton = ComponentGenerator::createButton(
		"goHomeButton",
		"Home",
		"",
		"bg-blue-600",
		"hover:bg-blue-700",
		"text-white",
		"px-4 py-2",
		"rounded",
		"transition-colors text-sm"
	);
	ComponentGenerator::addChild(&topBar, goHomeButton);
	ComponentGenerator::addChild(&container, topBar);

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


	/* Step list -- replaced by the "Selected Claim" panel below. Kept here
	   commented out in case we want to bring the flat step list back.
	ui::Component stepsHeading = ComponentGenerator::createText(
		"stepViewStepsHeading",
		"Debate Steps",
		"text-xs",
		"text-gray-500",
		"font-semibold uppercase tracking-wide",
		"mb-2"
	);

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

	ComponentGenerator::addChild(&leftColumn, stepsHeading);
	ComponentGenerator::addChild(&leftColumn, stepsContainer);
	*/

	ui::Component selectedClaimPanel = BuildSelectedClaimPanel(
		currentClaimId, collectionProto, userDb, fullDebateInfo.viewer_user_id()
	);
	ComponentGenerator::addChild(&leftColumn, selectedClaimPanel);

	// Render a single interactive paragraph tree for the entire debate,
	// underneath the step list within the left column.
	if (rootClaimId > 0) {
		ui::Component treeWrapper = ComponentGenerator::createContainer(
			"debateTreeWrapper",
			"w-full",
			"bg-gray-800/50",
			"p-4",
			"",
			"border border-gray-700",
			"rounded-lg",
			""
		);
		(*treeWrapper.mutable_css())["margin-top"] = "1rem";

		ui::Component treeHeader = ComponentGenerator::createText(
			"debateTreeHeader",
			"Paragraph Tree",
			"text-xs",
			"text-gray-500",
			"font-semibold uppercase tracking-wide",
			"mb-2"
		);
		ComponentGenerator::addChild(&treeWrapper, treeHeader);

		std::unordered_set<int> visited;
		visited.insert(rootClaimId);
		ui::Component treeRoot = BuildTreeNode(
			rootClaimId, fullDebateInfo, collectionProto,
			"debateTree", visited, currentClaimId
		);
		ComponentGenerator::addAttribute(&treeRoot, "data-tree-root", "true");
		ComponentGenerator::addChild(&treeWrapper, treeRoot);
		ComponentGenerator::addChild(&leftColumn, treeWrapper);
	}

	const int mapFocusClaimId = currentClaimId > 0
		? currentClaimId
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

	// Claim Parser placeholder — underneath the step view content.
	// Temporarily disabled.
	// ui::Component claimParser = ComponentGenerator::createClaimParser("stepViewClaimParser", "");
	// (*claimParser.mutable_css())["margin-top"] = "1rem";
	// ComponentGenerator::addChild(&container, claimParser);

	// Add Child / Challenge modals (from the Selected Claim panel's buttons)
	// reuse the same overlay rendering as the single-claim view -- they're
	// driven purely by the viewer's engagement state, not by which page
	// they're rendered on.
	rendering_info::DebatePageRenderingInfo debatePageInfo = FullDebatePageInfoParser::ParseFromUser(viewerUser, collectionProto);
	container = FullDebatePageGenerator::AddAppropriateOverlays(debatePageInfo, viewerUser, container);

	ui::Component* root = page.add_components();
	root->CopyFrom(container);
	return page;
}
