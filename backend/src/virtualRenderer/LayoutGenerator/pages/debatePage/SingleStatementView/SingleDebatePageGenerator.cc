#include "SingleDebatePageGenerator.h"
#include "../../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../../utils/Log.h"
#include "../../../../../utils/DemoMode.h"
#include "user.pb.h"
#include "user_engagement.pb.h"

namespace {
debate::ScopeType MapScopeType(rendering_info::ScopeType scopeType) {
    switch (scopeType) {
        case rendering_info::SINGLE_CLAIM:
            return debate::SINGLE_CLAIM;
        case rendering_info::FULL_DEBATE:
            return debate::FULL_DEBATE;
        default:
            return debate::SINGLE_CLAIM;
    }
}

// ClaimStatus is now shared — pass-through.
debate::ClaimStatus MapClaimStatus(debate::ClaimStatus status) { return status; }

ui::Component CreateStableActionSlot(
    const std::string& slotId,
    const ui::Component* action,
    const std::string& placeholderId
) {
    ui::Component slot = ComponentGenerator::createContainer(
        slotId,
        "w-full min-w-0",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    if (action != nullptr) {
        ui::Component actionCopy = *action;
        (*actionCopy.mutable_css())["width"] = "100%";
        ComponentGenerator::addChild(&slot, actionCopy);
    } else {
        ui::Component placeholder = ComponentGenerator::createContainer(
            placeholderId,
            "w-full h-10",
            "",
            "",
            "",
            "",
            "",
            ""
        );
        (*placeholder.mutable_css())["visibility"] = "hidden";
        (*placeholder.mutable_css())["pointer-events"] = "none";
        ComponentGenerator::addChild(&slot, placeholder);
    }

    return slot;
}
}

user_engagement::DebatingInfo_CurrentDebateAction_ActionType MapDebateAction(rendering_info::DebateActionType actionType) {
    switch (actionType) {
        case rendering_info::VIEWING_CLAIM:
            return user_engagement::DebatingInfo_CurrentDebateAction_ActionType_VIEWING_CLAIM;
        case rendering_info::ADDING_CHILD_CLAIM:
            return user_engagement::DebatingInfo_CurrentDebateAction_ActionType_ADDING_CHILD_CLAIM;
        case rendering_info::CONNECTING_CLAIMS:
            return user_engagement::DebatingInfo_CurrentDebateAction_ActionType_CONNECTING_CLAIMS;
        case rendering_info::CHALLENGING_CLAIM:
            return user_engagement::DebatingInfo_CurrentDebateAction_ActionType_CHALLENGING_CLAIM;
        case rendering_info::EDITING_CLAIM_DESCRIPTION:
            return user_engagement::DebatingInfo_CurrentDebateAction_ActionType_EDITING_CLAIM_DESCRIPTION;
        case rendering_info::EDITING_CLAIM:
            return user_engagement::DebatingInfo_CurrentDebateAction_ActionType_EDITING_CLAIM;
        case rendering_info::REPORTING_CLAIM:
            return user_engagement::DebatingInfo_CurrentDebateAction_ActionType_REPORTING_CLAIM;
        default:
            return user_engagement::DebatingInfo_CurrentDebateAction_ActionType_VIEWING_CLAIM;
    }
}


ui::Page DebatePageGenerator::GenerateSingleDebatePage(const rendering_info::DebatePageRenderingInfo& info, const user::User& userProto) {
    // later get user proto from database, for now just get from args
    ui::Page page;
    page.set_page_id("debate");
    page.set_title("Debate View: Debate ID " + std::to_string(userProto.engagement().debating_info().debate_id()) + " | commit: " GIT_COMMIT_HASH);
    ui::Component mainLayout;

    const auto scopeType = userProto.current_scope().scopetype();
    if (scopeType == debate::SINGLE_CLAIM) {
        mainLayout = GenerateSingleClaimLayout();
        Log::info("Generating single claim layout for user " + userProto.username() + " with scope type: " + std::to_string(scopeType));
    } else {
        mainLayout = GenerateSingleClaimLayout();
        Log::warn("Unknown scope type for user " + userProto.username() + ": " + std::to_string(scopeType) + ". Defaulting to single claim layout.");
    }

    // more individual functions for each part
    mainLayout = FillChildClaims(info, userProto, mainLayout);
    mainLayout = FillChallenges(info, userProto, mainLayout);
    mainLayout = FillCurrentClaimSection(info, userProto, mainLayout);
    mainLayout = AddAppropriateButtons(info, userProto, mainLayout);
    mainLayout = AddAppropriateOverlays(info, userProto, mainLayout);    

    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(mainLayout);
    return page; // test
}

ui::Component DebatePageGenerator::GenerateSingleClaimLayout() {
    // Main layout container
    ui::Component mainLayout = ComponentGenerator::createContainer(
        "mainLayout",
        "",
        "bg-gray-900",
        "p-4",
        "",
        "",
        "",
        "min-h-screen"
    );

    // Top section container
    ui::Component topSection = ComponentGenerator::createContainer(
        "topSection",
        "flex justify-between items-start",
        "",
        "",
        "mb-8",
        "",
        "",
        ""
    );

    // Left top section - navigation buttons
    ui::Component leftTopSection = ComponentGenerator::createContainer(
        "leftTopSection",
        "flex flex-col gap-2",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    ui::Component goToParentButton = ComponentGenerator::createButton(
        "goToParentButton",
        "Go to Parent",
        "",
        "bg-gray-600",
        "hover:bg-gray-700",
        "text-white",
        "px-4 py-2",
        "rounded",
        "transition-colors text-sm"
    );
    ComponentGenerator::addChild(&leftTopSection, goToParentButton);
    ComponentGenerator::addChild(&topSection, leftTopSection);

    // Focus section (center)
    ui::Component focusSection = ComponentGenerator::createContainer(
        "focusSection",
        "flex-1",
        "",
        "",
        "mx-8",
        "",
        "",
        ""
    );

    ui::Component focusingOnClaim = ComponentGenerator::createContainer(
        "focusingOnClaim",
        "text-center",
        "bg-gray-700",
        "py-4 px-6",
        "",
        "border",
        "rounded",
        ""
    );

    ui::Component focusLabel = ComponentGenerator::createText(
        "focusLabel",
        "Focusing on claim:",
        "text-sm",
        "text-white",
        "",
        "mb-2"
    );
    ComponentGenerator::addChild(&focusingOnClaim, focusLabel);

    ui::Component claimTitleContainer = ComponentGenerator::createContainer(
        "claimTitleContainer",
        "flex items-center justify-center gap-3",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    ui::Component currentClaimTitle = ComponentGenerator::createText(
        "currentClaimTitle",
        "",
        "text-lg",
        "text-white",
        "font-semibold",
        ""
    );
    ComponentGenerator::addChild(&claimTitleContainer, currentClaimTitle);
    ComponentGenerator::addChild(&focusingOnClaim, claimTitleContainer);
    ComponentGenerator::addChild(&focusSection, focusingOnClaim);
    ComponentGenerator::addChild(&topSection, focusSection);

    // Right top section - home button
    ui::Component rightTopSection = ComponentGenerator::createContainer(
        "rightTopSection",
        "flex flex-col gap-2",
        "",
        "",
        "",
        "",
        "",
        "w-64"
    );

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
    ComponentGenerator::addChild(&rightTopSection, goHomeButton);
    ComponentGenerator::addChild(&topSection, rightTopSection);
    ComponentGenerator::addChild(&mainLayout, topSection);

    // Content area
    ui::Component contentArea = ComponentGenerator::createContainer(
        "contentArea",
        "flex gap-8",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    // Left content - challenges
    ui::Component leftContent = ComponentGenerator::createContainer(
        "leftContent",
        "",
        "",
        "",
        "",
        "",
        "",
        "w-96"
    );

    ui::Component challengesBox = ComponentGenerator::createContainer(
        "challengesBox",
        "",
        "bg-gray-800",
        "p-6",
        "",
        "border-2 border-gray-700",
        "rounded",
        "min-h-64"
    );

    ui::Component challengesText = ComponentGenerator::createText(
        "challengesText",
        "Challenges:",
        "text-lg",
        "text-white",
        "font-semibold",
        "mb-4"
    );
    ComponentGenerator::addChild(&challengesBox, challengesText);

    ui::Component challengesContainer = ComponentGenerator::createContainer(
        "challengesContainer",
        "flex flex-col gap-3",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    ComponentGenerator::addChild(&challengesBox, challengesContainer);
    ComponentGenerator::addChild(&leftContent, challengesBox);
    ComponentGenerator::addChild(&contentArea, leftContent);

    // Center content
    ui::Component centerContent = ComponentGenerator::createContainer(
        "centerContent",
        "flex-1",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    // Description box
    ui::Component descriptionBox = ComponentGenerator::createContainer(
        "descriptionBox",
        "",
        "bg-gray-800",
        "p-6",
        "mb-8",
        "border-2 border-gray-700",
        "rounded",
        ""
    );

    ui::Component descriptionTitle = ComponentGenerator::createText(
        "descriptionTitle",
        "Claim Description:",
        "text-lg",
        "text-white",
        "font-semibold",
        "mb-4"
    );
    ComponentGenerator::addChild(&descriptionBox, descriptionTitle);

    ui::Component descriptionContent = ComponentGenerator::createText(
        "descriptionContent",
        "",
        "",
        "text-white",
        "",
        "leading-relaxed mb-6"
    );
    ComponentGenerator::addChild(&descriptionBox, descriptionContent);

    ui::Component descriptionActions = ComponentGenerator::createContainer(
        "descriptionActions",
        "flex flex-wrap gap-3",
        "",
        "",
        "",
        "",
        "",
        ""
    );
    ComponentGenerator::addChild(&descriptionBox, descriptionActions);
    ComponentGenerator::addChild(&centerContent, descriptionBox);

    // Child arguments section
    ui::Component childArgumentsSection = ComponentGenerator::createContainer(
        "childArgumentsSection",
        "",
        "bg-gray-800",
        "p-6",
        "",
        "border-2 border-gray-700",
        "rounded",
        "overflow-auto"
    );

    ui::Component childArgumentsTitle = ComponentGenerator::createText(
        "childArgumentsTitle",
        "Proof:",
        "text-lg",
        "text-white",
        "font-semibold",
        "mb-6"
    );
    ComponentGenerator::addChild(&childArgumentsSection, childArgumentsTitle);

    ui::Component childArgumentsGrid = ComponentGenerator::createContainer(
        "childArgumentsGrid",
        "flex flex-wrap items-start gap-4",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    ComponentGenerator::addChild(&childArgumentsSection, childArgumentsGrid);
    ComponentGenerator::addChild(&centerContent, childArgumentsSection);
    ComponentGenerator::addChild(&contentArea, centerContent);

    // Right content - guide and report
    ui::Component rightContent = ComponentGenerator::createContainer(
        "rightContent",
        "",
        "",
        "",
        "mt-16",
        "",
        "",
        "w-80"
    );

    // Container for modify actions (will be filled by AddAppropriateButtons)
    ui::Component modifyActionsContainer = ComponentGenerator::createContainer(
        "modifyActionsContainer",
        "mb-4",
        "",
        "",
        "",
        "",
        "",
        ""
    );
    ComponentGenerator::addChild(&rightContent, modifyActionsContainer);

    if (!demo_mode::disableClaimHistoryGuideAndReport) {
        // Claim History Button
        ui::Component claimHistoryButton = ComponentGenerator::createButton(
            "claimHistoryButton",
            "See Claim History",
            "",
            "bg-purple-600",
            "hover:bg-purple-700",
            "text-white",
            "px-4 py-2",
            "rounded",
            "w-full transition-colors mb-6"
        );
        ComponentGenerator::addChild(&rightContent, claimHistoryButton);

        ui::Component guidanceBox = ComponentGenerator::createContainer(
            "guidanceBox",
            "",
            "bg-gray-800",
            "p-6",
            "mb-6",
            "border-2 border-gray-700",
            "rounded",
            "min-h-64"
        );

        ui::Component guidanceText = ComponentGenerator::createText(
            "guidanceText",
            "Guide:",
            "text-lg",
            "text-white",
            "font-semibold",
            ""
        );
        ComponentGenerator::addChild(&guidanceBox, guidanceText);
        ComponentGenerator::addChild(&rightContent, guidanceBox);

        ui::Component reportSection = ComponentGenerator::createContainer(
            "reportSection",
            "",
            "bg-gray-800",
            "p-6",
            "",
            "border-2 border-gray-700",
            "rounded",
            ""
        );

        ui::Component reportTitle = ComponentGenerator::createText(
            "reportTitle",
            "Think this is wrong? File a claim report.",
            "text-sm",
            "text-white",
            "",
            "mb-6"
        );
        ComponentGenerator::addChild(&reportSection, reportTitle);

        ui::Component reportButton = ComponentGenerator::createButton(
            "reportButton",
            "Report Claim",
            "",
            "bg-red-600",
            "hover:bg-red-700",
            "text-white",
            "px-4 py-2",
            "rounded",
            "w-full transition-colors"
        );
        ComponentGenerator::addChild(&reportSection, reportButton);
        ComponentGenerator::addChild(&rightContent, reportSection);
    }
    ComponentGenerator::addChild(&contentArea, rightContent);
    ComponentGenerator::addChild(&mainLayout, contentArea);

    return mainLayout;
}

ui::Component DebatePageGenerator::FillChildClaims(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout) {
    // Extract data from user
    int currentUserId = user.user_id();
    user_engagement::DebatingInfo debatingInfo = user.engagement().debating_info();
    Log::debug(
        "[DebatePageGenerator] FillChildClaims received: user_links_count=" + std::to_string(debatingInfo.links_size()) +
        ", rendering_links_count=" + std::to_string(info.links_size())
    );
    Log::debug(
        "[DebatePageGenerator] child_claims received=" + std::to_string(info.children_claims_size()) +
        ", current_claim_id=" + std::to_string(debatingInfo.current_claim().id()) +
        ", current_claim_sentence=\"" + debatingInfo.current_claim().sentence() + "\""
    );
    for (int i = 0; i < info.children_claims_size(); i++) {
        const rendering_info::ClaimRenderInfo& childClaim = info.children_claims(i);
        Log::debug(
            "[DebatePageGenerator] child_claim[" + std::to_string(i) + "] id=" + std::to_string(childClaim.id()) +
            ", sentence=\"" + childClaim.sentence() + "\"" +
            ", creator_id=" + std::to_string(childClaim.creator_id()) +
            ", status=" + std::to_string(childClaim.status())
        );
    }
    for (int i = 0; i < debatingInfo.links_size(); ++i) {
        const user_engagement::LinkInfo& link = debatingInfo.links(i);
        Log::debug(
            "[DebatePageGenerator] user link[" + std::to_string(i) + "] id=" + std::to_string(link.id()) +
            ", from=" + std::to_string(link.connect_from()) +
            ", to=" + std::to_string(link.connect_to()) +
            ", creator_id=" + std::to_string(link.creator_id()) +
            ", connection=\"" + link.connection() + "\""
        );
    }
    for (int i = 0; i < info.links_size(); ++i) {
        const rendering_info::LinkRenderInfo& link = info.links(i);
        Log::debug(
            "[DebatePageGenerator] rendering_info link[" + std::to_string(i) + "] id=" + std::to_string(link.id()) +
            ", from=" + std::to_string(link.connect_from()) +
            ", to=" + std::to_string(link.connect_to()) +
            ", creator_id=" + std::to_string(link.creator_id()) +
            ", connection=\"" + link.connection() + "\""
        );
    }
    Log::info("[DebatePageGenerator] Connecting Info: " + std::string(debatingInfo.connecting_info().connecting() ? "true" : "false") + 
              ", fromClaimId=" + std::to_string(debatingInfo.connecting_info().from_claim_id()) + 
              ", toClaimId=" + std::to_string(debatingInfo.connecting_info().to_claim_id()) + 
              ", openedConnectModal=" + std::string(debatingInfo.connecting_info().opened_connect_modal() ? "true" : "false"));
    int currentClaimId = debatingInfo.current_claim().id();
    std::string claim = debatingInfo.current_claim().sentence();
    Log::info(
        "[DebatePageGenerator] FillChildClaims state: currentUserId=" + std::to_string(currentUserId) +
        ", currentClaimId=" + std::to_string(currentClaimId) +
        ", currentAction=" + std::to_string(debatingInfo.current_debate_action().action_type()) +
        ", modifyingCurrentClaim=" + std::string(debatingInfo.modifying_current_claim() ? "true" : "false") +
        ", connecting=" + std::string(debatingInfo.connecting_info().connecting() ? "true" : "false") +
        ", fromClaimId=" + std::to_string(debatingInfo.connecting_info().from_claim_id()) +
        ", toClaimId=" + std::to_string(debatingInfo.connecting_info().to_claim_id()) +
        ", openedConnectModal=" + std::string(debatingInfo.connecting_info().opened_connect_modal() ? "true" : "false")
    );
    
    std::vector<std::tuple<std::string,std::string,int,debate::ClaimStatus,std::vector<rendering_info::UserStatus>>> childClaimInfo; // id, sentence, creator_id, status, user_statuses
    for (int i = 0; i < info.children_claims_size(); i++) {
        const rendering_info::ClaimRenderInfo& childClaim = info.children_claims(i);
        std::vector<rendering_info::UserStatus> userStatuses;
        for (int j = 0; j < childClaim.user_statuses_size(); j++) {
            userStatuses.push_back(childClaim.user_statuses(j));
        }
        childClaimInfo.push_back({std::to_string(childClaim.id()), childClaim.sentence(), childClaim.creator_id(), childClaim.status(), userStatuses});
    }
    Log::debug("[DebatePageGenerator] prepared childClaimInfo count=" + std::to_string(childClaimInfo.size()));
    for (size_t i = 0; i < childClaimInfo.size(); i++) {
        Log::debug(
            "[DebatePageGenerator] prepared childClaimInfo[" + std::to_string(i) + "] id=" + std::get<0>(childClaimInfo[i]) +
            ", sentence=\"" + std::get<1>(childClaimInfo[i]) + "\"" +
            ", creator_id=" + std::to_string(std::get<2>(childClaimInfo[i])) +
            ", status=" + std::to_string(std::get<3>(childClaimInfo[i]))
        );
    }
    
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> linkInfo;
    for (int i = 0; i < info.links_size(); i++) {
        const rendering_info::LinkRenderInfo& link = info.links(i);
        linkInfo.push_back({std::to_string(link.id()), std::to_string(link.connect_from()), std::to_string(link.connect_to()), link.connection()});
    }
    Log::debug("[DebatePageGenerator] prepared linkInfo count=" + std::to_string(linkInfo.size()));
    
    bool connecting = debatingInfo.connecting_info().connecting();
    std::string fromClaimId = std::to_string(debatingInfo.connecting_info().from_claim_id());
    
    std::vector<int> currentChallengedClaimIds;
    std::vector<int> currentChallengedLinkIds;
    for (int i = 0; i < debatingInfo.challenging_info().claim_ids_size(); i++) {
        currentChallengedClaimIds.push_back(debatingInfo.challenging_info().claim_ids(i));
    }
    for (int i = 0; i < debatingInfo.challenging_info().link_ids_size(); i++) {
        currentChallengedLinkIds.push_back(debatingInfo.challenging_info().link_ids(i));
    }
    
    // Check if we're in challenging mode
    bool challengingClaim = (debatingInfo.current_debate_action().action_type() == 
                            user_engagement::DebatingInfo_CurrentDebateAction_ActionType_CHALLENGING_CLAIM);
    
    // Check if user is in modify mode
    bool modifyingCurrentClaim = debatingInfo.modifying_current_claim();

    if (demo_mode::kReadOnlyMode) {
        challengingClaim = false;
        modifyingCurrentClaim = false;
    }

    // Find the childArgumentsGrid container within mainLayout
    ui::Component* childArgumentsGrid = nullptr;
    
    // Navigate: mainLayout -> contentArea -> centerContent -> childArgumentsSection -> childArgumentsGrid
    for (int i = 0; i < mainLayout.children_size(); i++) {
        ui::Component* contentArea = mainLayout.mutable_children(i);
        if (contentArea->id() == "contentArea") {
            for (int j = 0; j < contentArea->children_size(); j++) {
                ui::Component* centerContent = contentArea->mutable_children(j);
                if (centerContent->id() == "centerContent") {
                    for (int k = 0; k < centerContent->children_size(); k++) {
                        ui::Component* childArgumentsSection = centerContent->mutable_children(k);
                        if (childArgumentsSection->id() == "childArgumentsSection") {
                            for (int l = 0; l < childArgumentsSection->children_size(); l++) {
                                ui::Component* grid = childArgumentsSection->mutable_children(l);
                                if (grid->id() == "childArgumentsGrid") {
                                    childArgumentsGrid = grid;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    if (childArgumentsGrid == nullptr) {
        return mainLayout;
    }

    // Populate child claims
    for (size_t i = 0; i < childClaimInfo.size(); i++) {
        std::string claimId = std::get<0>(childClaimInfo[i]);
        std::string claimSentence = std::get<1>(childClaimInfo[i]);
        int claimCreatorId = std::get<2>(childClaimInfo[i]);
        debate::ClaimStatus claimStatus = std::get<3>(childClaimInfo[i]);
        bool userOwnsChildClaim = (currentUserId == claimCreatorId);
        Log::debug(
            "[DebatePageGenerator] Rendering child claim: id=" + claimId +
            ", creatorId=" + std::to_string(claimCreatorId) +
            ", userOwnsChildClaim=" + std::string(userOwnsChildClaim ? "true" : "false") +
            ", modifyingCurrentClaim=" + std::string(modifyingCurrentClaim ? "true" : "false") +
            ", connecting=" + std::string(connecting ? "true" : "false") +
            ", fromClaimId=" + fromClaimId
        );
        
        // Determine border color based on status
        std::string borderColor;
        switch (claimStatus) {
            case debate::ClaimStatus::FALSE_CLAIM:
                borderColor = "border-2 border-red-500";
                break;
            case debate::ClaimStatus::TRUE_CLAIM:
                borderColor = "border-2 border-green-500";
                break;
            case debate::ClaimStatus::UNDETERMINED:
                borderColor = "border-2 border-gray-600";
                break;
            default:
                borderColor = "border-2 border-purple-700"; // unknown status
                break;
        }

        // Determine status label text and matching text color
        std::string statusText;
        std::string statusTextColor;
        switch (claimStatus) {
            case debate::ClaimStatus::FALSE_CLAIM:
                statusText = "False";
                statusTextColor = "text-red-500";
                break;
            case debate::ClaimStatus::TRUE_CLAIM:
                statusText = "True";
                statusTextColor = "text-green-500";
                break;
            case debate::ClaimStatus::UNDETERMINED:
                statusText = "Undetermined";
                statusTextColor = "text-gray-400";
                break;
            default:
                statusText = "Unknown";
                statusTextColor = "text-purple-500";
                break;
        }
        
        std::string nodeId = "childNode_" + claimId;
        ui::Component childNode = ComponentGenerator::createContainer(
            nodeId,
            "",
            "bg-gray-700",
            "p-4",
            "",
            borderColor,
            "rounded",
            "w-80 flex-shrink-0"
        );

        // Per-user status rectangles above the claim sentence
        if (std::get<4>(childClaimInfo[i]).size() > 0) {
            ui::Component userStatusRow = ComponentGenerator::createContainer(
                nodeId + "UserStatusRow",
                "flex flex-wrap gap-1 mb-2",
                "",
                "",
                "",
                "",
                "",
                ""
            );
            for (const auto& us : std::get<4>(childClaimInfo[i])) {
                std::string rectColor;
                switch (us.status()) {
                    case debate::ClaimStatus::TRUE_CLAIM:
                        rectColor = "bg-green-500";
                        break;
                    case debate::ClaimStatus::FALSE_CLAIM:
                        rectColor = "bg-red-500";
                        break;
                    case debate::ClaimStatus::UNDETERMINED:
                        rectColor = "bg-gray-500";
                        break;
                    default:
                        rectColor = "bg-purple-500";
                        break;
                }
                ui::Component userBadge = ComponentGenerator::createContainer(
                    nodeId + "_" + us.username() + "_badge",
                    "flex items-center",
                    "",
                    "px-1 py-0.5",
                    "",
                    "rounded",
                    "",
                    "mr-1"
                );
                ui::Component colorRect = ComponentGenerator::createContainer(
                    nodeId + "_" + us.username() + "_rect",
                    "w-3 h-3 rounded-sm " + rectColor,
                    "",
                    "",
                    "",
                    "",
                    "mr-1",
                    ""
                );
                ComponentGenerator::addChild(&userBadge, colorRect);
                ui::Component nameText = ComponentGenerator::createText(
                    nodeId + "_" + us.username() + "_name",
                    us.username() + ": ",
                    "text-xs",
                    "text-gray-300",
                    "",
                    ""
                );
                ComponentGenerator::addChild(&userBadge, nameText);
                ComponentGenerator::addChild(&userStatusRow, userBadge);
            }
            ComponentGenerator::addChild(&childNode, userStatusRow);
        }

        ui::Component childNodeTitle = ComponentGenerator::createText(
            nodeId + "Title",
            claimSentence,
            "",
            "text-white",
            "",
            "mb-3"
        );
        ComponentGenerator::addChild(&childNode, childNodeTitle);

        // Display connections from this claim
        for (const auto& link : linkInfo) {
            if (std::get<1>(link) == claimId) {
                // This claim has a connection - find the target claim sentence
                std::string targetSentence = "";
                std::string linkId = std::get<0>(link);
                std::string targetId = std::get<2>(link);
                std::string connection = std::get<3>(link);
                
                // Check if target is current claim
                if ((targetId) == std::to_string(currentClaimId)) {
                    targetSentence = claim;
                } else {
                    // Search in child claims
                    for (const auto& childClaim : childClaimInfo) {
                        if (std::get<0>(childClaim) == targetId) {
                            targetSentence = std::get<1>(childClaim);
                            break;
                        }
                    }
                }
                
                // Create link container with border and background
                ui::Component linkContainer = ComponentGenerator::createContainer(
                    nodeId + "_linkContainer_" + linkId,
                    "flex justify-between items-start",
                    "bg-purple-900/30",
                    "p-2",
                    "mb-2",
                    "border border-purple-500",
                    "rounded",
                    ""
                );

                // Text container for the link info
                ui::Component linkTextContainer = ComponentGenerator::createContainer(
                    nodeId + "_linkTextContainer_" + linkId,
                    "flex-1",
                    "",
                    "",
                    "",
                    "",
                    "",
                    ""
                );

                // Create "Connects to:" text
                ui::Component connectsToLabel = ComponentGenerator::createText(
                    nodeId + "_connectsToLabel_" + linkId,
                    "Connects to: " + targetSentence,
                    "text-xs",
                    "text-purple-300",
                    "",
                    ""
                );
                ComponentGenerator::addChild(&linkTextContainer, connectsToLabel);

                // Create "Connection:" text
                ui::Component connectionLabel = ComponentGenerator::createText(
                    nodeId + "_connectionLabel_" + linkId,
                    "Connection: " + connection,
                    "text-xs",
                    "text-purple-300",
                    "",
                    ""
                );
                ComponentGenerator::addChild(&linkTextContainer, connectionLabel);

                ComponentGenerator::addChild(&linkContainer, linkTextContainer);

                // Button container for link actions
                ui::Component linkButtonContainer = ComponentGenerator::createContainer(
                    nodeId + "_linkButtonContainer_" + linkId,
                    "grid grid-cols-2 gap-1",
                    "",
                    "",
                    "",
                    "",
                    "",
                    ""
                );

                // Delete link button - only show if user owns the child claim (the "from" claim) and is modifying
                ui::Component deleteLinkButton;
                bool hasDeleteLinkButton = false;
                if (userOwnsChildClaim && modifyingCurrentClaim) {
                    deleteLinkButton = ComponentGenerator::createButton(
                        "deleteLinkButton_" + linkId,
                        "Delete",
                        "",
                        "bg-red-600",
                        "hover:bg-red-700",
                        "text-white",
                        "px-2 py-1",
                        "rounded",
                        "w-full transition-colors text-xs"
                    );
                    hasDeleteLinkButton = true;
                }

                // Challenge buttons - show if user is challenging claim
                ui::Component challengeLinkButton;
                bool hasChallengeLinkButton = false;
                if (challengingClaim) {
                    // Check if this link is in the current challenge
                    bool linkInChallenge = std::find(currentChallengedLinkIds.begin(), 
                                                     currentChallengedLinkIds.end(), 
                                                     std::stoi(linkId)) != currentChallengedLinkIds.end();
                    
                    if (linkInChallenge) {
                        challengeLinkButton = ComponentGenerator::createButton(
                            "removeLinkFromChallengeButton_" + linkId,
                            "Remove from Challenge",
                            "",
                            "bg-orange-600",
                            "hover:bg-orange-700",
                            "text-white",
                            "px-2 py-1",
                            "rounded",
                            "w-full transition-colors text-xs"
                        );
                        hasChallengeLinkButton = true;
                    } else {
                        challengeLinkButton = ComponentGenerator::createButton(
                            "addLinkToChallengeButton_" + linkId,
                            "Add to Challenge",
                            "",
                            "bg-orange-600",
                            "hover:bg-orange-700",
                            "text-white",
                            "px-2 py-1",
                            "rounded",
                            "w-full transition-colors text-xs"
                        );
                        hasChallengeLinkButton = true;
                    }
                }

                ComponentGenerator::addChild(
                    &linkButtonContainer,
                    CreateStableActionSlot(
                        nodeId + "_linkDeleteSlot_" + linkId,
                        hasDeleteLinkButton ? &deleteLinkButton : nullptr,
                        nodeId + "_linkDeleteSlotPlaceholder_" + linkId
                    )
                );
                ComponentGenerator::addChild(
                    &linkButtonContainer,
                    CreateStableActionSlot(
                        nodeId + "_linkChallengeSlot_" + linkId,
                        hasChallengeLinkButton ? &challengeLinkButton : nullptr,
                        nodeId + "_linkChallengeSlotPlaceholder_" + linkId
                    )
                );

                ComponentGenerator::addChild(&linkContainer, linkButtonContainer);
                ComponentGenerator::addChild(&childNode, linkContainer);
            }
        }

        ui::Component childNodeButtonContainer = ComponentGenerator::createContainer(
            nodeId + "ButtonContainer",
            "grid grid-cols-2 gap-2",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        ui::Component childNodeButton = ComponentGenerator::createButton(
            "viewChildNodeButton_" + claimId,
            "View Claim",
            "",
            "bg-blue-600",
            "hover:bg-blue-700",
            "text-white",
            "px-4 py-2",
            "rounded",
            "w-full transition-colors text-sm"
        );
        ComponentGenerator::addChild(
            &childNodeButtonContainer,
            CreateStableActionSlot(
                nodeId + "_viewClaimSlot",
                &childNodeButton,
                nodeId + "_viewClaimSlotPlaceholder"
            )
        );

        // Challenge buttons - show if user is challenging claim
        ui::Component claimChallengeButton;
        bool hasClaimChallengeButton = false;
        if (challengingClaim) {
            // Check if this claim is in the current challenge
            bool claimInChallenge = std::find(currentChallengedClaimIds.begin(), 
                                             currentChallengedClaimIds.end(), 
                                             std::stoi(claimId)) != currentChallengedClaimIds.end();
            
            if (claimInChallenge) {
                claimChallengeButton = ComponentGenerator::createButton(
                    "removeClaimFromChallengeButton_" + claimId,
                    "Remove from Challenge",
                    "",
                    "bg-orange-600",
                    "hover:bg-orange-700",
                    "text-white",
                    "px-4 py-2",
                    "rounded",
                    "w-full transition-colors text-sm"
                );
                hasClaimChallengeButton = true;
            } else {
                claimChallengeButton = ComponentGenerator::createButton(
                    "addClaimToChallengeButton_" + claimId,
                    "Add to Challenge",
                    "",
                    "bg-orange-600",
                    "hover:bg-orange-700",
                    "text-white",
                    "px-4 py-2",
                    "rounded",
                    "w-full transition-colors text-sm"
                );
                hasClaimChallengeButton = true;
            }
        }

        ComponentGenerator::addChild(
            &childNodeButtonContainer,
            CreateStableActionSlot(
                nodeId + "_claimChallengeSlot",
                hasClaimChallengeButton ? &claimChallengeButton : nullptr,
                nodeId + "_claimChallengeSlotPlaceholder"
            )
        );

        // Only show connection and delete buttons if user owns the claim and is modifying
        Log::debug("[DebatePageGenerator] userOwnsChildClaim=" + std::string(userOwnsChildClaim ? "true" : "false") + ", modifyingCurrentClaim=" + std::string(modifyingCurrentClaim ? "true" : "false") + ", connecting=" + std::string(connecting ? "true" : "false"));
        ui::Component connectionActionButton;
        bool hasConnectionActionButton = false;
        ui::Component deleteChildClaimButton;
        bool hasDeleteChildClaimButton = false;
        if (userOwnsChildClaim && modifyingCurrentClaim) {
            // Add connection buttons based on connecting state
            if (connecting) {
                // If we're connecting and this is the FROM claim, show Cancel button
                Log::info("[DebatePageGenerator] Currently connecting claims. fromClaimId=" + fromClaimId + ", child claimId=" + claimId);
                if (claimId == fromClaimId) {
                    Log::debug("[DebatePageGenerator] Child claim " + claimId + " is the from claim; rendering Cancel Connection button");
                    connectionActionButton = ComponentGenerator::createButton(
                        "cancelConnectClaimsButton_" + claimId,
                        "Cancel Connection",
                        "",
                        "bg-gray-600",
                        "hover:bg-gray-700",
                        "text-white",
                        "px-4 py-2",
                        "rounded",
                        "w-full transition-colors text-sm"
                    );
                    hasConnectionActionButton = true;
                } else {
                    // For all other claims, show Connect To button
                    Log::debug("[DebatePageGenerator] Child claim " + claimId + " is not the from claim; rendering Connect To button");
                    connectionActionButton = ComponentGenerator::createButton(
                        "connectToClaimButton_" + claimId,
                        "Connect To",
                        "",
                        "bg-purple-600",
                        "hover:bg-purple-700",
                        "text-white",
                        "px-4 py-2",
                        "rounded",
                        "w-full transition-colors text-sm"
                    );
                    hasConnectionActionButton = true;
                }
            } else {
                // If we're not connecting, show Connect From button
                Log::debug("[DebatePageGenerator] Child claim " + claimId + " is not connecting; rendering Connect From button");
                connectionActionButton = ComponentGenerator::createButton(
                    "connectFromClaimButton_" + claimId,
                    "Connect From",
                    "",
                    "bg-purple-600",
                    "hover:bg-purple-700",
                    "text-white",
                    "px-4 py-2",
                    "rounded",
                    "w-full transition-colors text-sm"
                );
                hasConnectionActionButton = true;
            }

            Log::debug("[DebatePageGenerator] Child claim " + claimId + " rendering Delete button alongside connection controls");
            deleteChildClaimButton = ComponentGenerator::createButton(
                "deleteChildClaimButton_" + claimId,
                "Delete",
                "",
                "bg-red-600",
                "hover:bg-red-700",
                "text-white",
                "px-4 py-2",
                "rounded",
                "w-full transition-colors text-sm"
            );
            hasDeleteChildClaimButton = true;
        }

        ComponentGenerator::addChild(
            &childNodeButtonContainer,
            CreateStableActionSlot(
                nodeId + "_connectionActionSlot",
                hasConnectionActionButton ? &connectionActionButton : nullptr,
                nodeId + "_connectionActionSlotPlaceholder"
            )
        );
        ComponentGenerator::addChild(
            &childNodeButtonContainer,
            CreateStableActionSlot(
                nodeId + "_deleteChildClaimSlot",
                hasDeleteChildClaimButton ? &deleteChildClaimButton : nullptr,
                nodeId + "_deleteChildClaimSlotPlaceholder"
            )
        );

        ComponentGenerator::addChild(&childNode, childNodeButtonContainer);
        ComponentGenerator::addChild(childArgumentsGrid, childNode);
    }

    return mainLayout;
}
ui::Component DebatePageGenerator::FillChallenges(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout){
    // Extract data
    int currentUserId = user.user_id();
    
    std::vector<std::tuple<std::string, std::string, int, int>> challengesInfo;
    for (int i = 0; i < info.current_challenges_size(); i++) {
        const rendering_info::ChallengeRenderInfo& challenge = info.current_challenges(i);
        challengesInfo.push_back({std::to_string(challenge.id()), challenge.sentence(), challenge.creator_id(), challenge.challenge_link_id()});
    }

    // Find challengesContainer
    ui::Component* challengesContainer = nullptr;
    for (int i = 0; i < mainLayout.children_size(); i++) {
        ui::Component* contentArea = mainLayout.mutable_children(i);
        if (contentArea->id() == "contentArea") {
            for (int j = 0; j < contentArea->children_size(); j++) {
                ui::Component* leftContent = contentArea->mutable_children(j);
                if (leftContent->id() == "leftContent") {
                    for (int k = 0; k < leftContent->children_size(); k++) {
                        ui::Component* challengesBox = leftContent->mutable_children(k);
                        if (challengesBox->id() == "challengesBox") {
                            for (int l = 0; l < challengesBox->children_size(); l++) {
                                ui::Component* container = challengesBox->mutable_children(l);
                                if (container->id() == "challengesContainer") {
                                    challengesContainer = container;
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    if (challengesContainer == nullptr) {
        return mainLayout;
    }

    // Populate challenges
    for (const auto& challenge : challengesInfo) {
        std::string challengedClaimId = std::get<0>(challenge);
        std::string challengeSentence = std::get<1>(challenge);
        int challengeCreatorId = std::get<2>(challenge);
        int challengeLinkId = std::get<3>(challenge);
        bool userOwnsChallenge = (currentUserId == challengeCreatorId);

        // Challenges no longer have their own status — use default styling.
        std::string bgColor = "bg-blue-600";
        std::string borderColor = "border-2 border-blue-500";
        std::string buttonBgColor = "bg-blue-700";
        std::string buttonHoverColor = "hover:bg-blue-800";
        std::string statusText = "Active";
        std::string statusTextColor = "text-blue-300";
        
        ui::Component challengeNode = ComponentGenerator::createContainer(
            "challengeNode_" + challengedClaimId,
            "",
            bgColor,
            "p-4",
            "",
            borderColor,
            "rounded",
            ""
        );

        // Status label above the challenge sentence
        ui::Component statusLabel = ComponentGenerator::createText(
            "challengeStatus_" + challengedClaimId,
            "Status: " + statusText,
            "text-xs",
            statusTextColor,
            "font-semibold",
            "mb-2"
        );
        ComponentGenerator::addChild(&challengeNode, statusLabel);

        ui::Component challengeSentenceText = ComponentGenerator::createText(
            "challengeSentence_" + challengedClaimId,
            challengeSentence,
            "",
            "text-white",
            "",
            "mb-3"
        );
        ComponentGenerator::addChild(&challengeNode, challengeSentenceText);

        ui::Component challengeButtonContainer = ComponentGenerator::createContainer(
            "challengeButtonContainer_" + challengedClaimId,
            "grid grid-cols-2 gap-2",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        ui::Component viewChallengeButton = ComponentGenerator::createButton(
            "viewChallengeButton_" + challengedClaimId,
            "View Challenged Claim",
            "",
            buttonBgColor,
            buttonHoverColor,
            "text-white",
            "px-4 py-2",
            "rounded",
            "w-full transition-colors text-sm"
        );
        ComponentGenerator::addChild(
            &challengeButtonContainer,
            CreateStableActionSlot(
                "challengeViewSlot_" + challengedClaimId,
                &viewChallengeButton,
                "challengeViewSlotPlaceholder_" + challengedClaimId
            )
        );

        // Concede button: visible when the current user's claim is being challenged
        if (!userOwnsChallenge && !demo_mode::kReadOnlyMode) {
            ui::Component concedeButton = ComponentGenerator::createButton(
                "concedeChallengeButton_" + std::to_string(challengeLinkId),
                "Concede",
                "",
                "bg-red-700",
                "hover:bg-red-800",
                "text-white",
                "px-4 py-2",
                "rounded",
                "w-full transition-colors text-sm"
            );
            ComponentGenerator::addChild(
                &challengeButtonContainer,
                CreateStableActionSlot(
                    "challengeConcedeSlot_" + challengedClaimId,
                    &concedeButton,
                    "challengeConcedeSlotPlaceholder_" + challengedClaimId
                )
            );
        } else {
            // When user owns the challenge, show a spacer for the concede slot
            ui::Component concedeSpacer = ComponentGenerator::createText(
                "concedeSpacer_" + challengedClaimId,
                "",
                "hidden"
            );
            ComponentGenerator::addChild(
                &challengeButtonContainer,
                CreateStableActionSlot(
                    "challengeConcedeSlot_" + challengedClaimId,
                    &concedeSpacer,
                    "challengeConcedeSlotPlaceholder_" + challengedClaimId
                )
            );
        }

        ui::Component deleteChallengeButton;
        bool hasDeleteChallengeButton = false;

        if (userOwnsChallenge && !demo_mode::kReadOnlyMode) {
            deleteChallengeButton = ComponentGenerator::createButton(
                "deleteChallengeButton_" + challengedClaimId,
                "Delete",
                "",
                "bg-red-600",
                "hover:bg-red-700",
                "text-white",
                "px-4 py-2",
                "rounded",
                "w-full transition-colors text-sm"
            );
            hasDeleteChallengeButton = true;
        }

        ComponentGenerator::addChild(
            &challengeButtonContainer,
            CreateStableActionSlot(
                "challengeDeleteSlot_" + challengedClaimId,
                hasDeleteChallengeButton ? &deleteChallengeButton : nullptr,
                "challengeDeleteSlotPlaceholder_" + challengedClaimId
            )
        );

        ComponentGenerator::addChild(
            &challengeButtonContainer,
            CreateStableActionSlot(
                "challengeSpacerSlot_" + challengedClaimId,
                nullptr,
                "challengeSpacerSlotPlaceholder_" + challengedClaimId
            )
        );

        ComponentGenerator::addChild(&challengeNode, challengeButtonContainer);
        ComponentGenerator::addChild(challengesContainer, challengeNode);
    }

    return mainLayout;
}
ui::Component DebatePageGenerator::FillCurrentClaimSection(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout){
    // Extract data
    int currentUserId = user.user_id();
    user_engagement::DebatingInfo debatingInfo = user.engagement().debating_info();
    int currentClaimCreatorId = debatingInfo.current_claim().creator_id();
    bool modifyingCurrentClaim = debatingInfo.modifying_current_claim();
    std::string claim = debatingInfo.current_claim().sentence();
    std::string currentClaimDescription = debatingInfo.current_claim_description();
    
    bool editingDescription = (debatingInfo.current_debate_action().action_type() == 
                               user_engagement::DebatingInfo_CurrentDebateAction_ActionType_EDITING_CLAIM_DESCRIPTION);
    bool editingClaim = (debatingInfo.current_debate_action().action_type() == 
                         user_engagement::DebatingInfo_CurrentDebateAction_ActionType_EDITING_CLAIM);

    if (demo_mode::kReadOnlyMode) {
        modifyingCurrentClaim = false;
        editingDescription = false;
        editingClaim = false;
    }

    // Update the current claim title in focusingOnClaim
    for (int i = 0; i < mainLayout.children_size(); i++) {
        ui::Component* topSection = mainLayout.mutable_children(i);
        if (topSection->id() == "topSection") {
            for (int j = 0; j < topSection->children_size(); j++) {
                ui::Component* focusSection = topSection->mutable_children(j);
                if (focusSection->id() == "focusSection") {
                    for (int k = 0; k < focusSection->children_size(); k++) {
                        ui::Component* focusingOnClaim = focusSection->mutable_children(k);
                        if (focusingOnClaim->id() == "focusingOnClaim") {
                            for (int l = 0; l < focusingOnClaim->children_size(); l++) {
                                ui::Component* claimTitleContainer = focusingOnClaim->mutable_children(l);
                                if (claimTitleContainer->id() == "claimTitleContainer") {
                                    // Clear existing children
                                    claimTitleContainer->clear_children();
                                    
                                    if (editingClaim) {
                                        // Show input for editing claim title
                                        ui::Component editClaimInput = ComponentGenerator::createInput(
                                            "editClaimInput",
                                            claim,
                                            "claimTitle",
                                            "bg-gray-700",
                                            "text-white",
                                            "border-gray-600",
                                            "p-2",
                                            "rounded",
                                            "text-lg font-semibold flex-1"
                                        );
                                        ComponentGenerator::addChild(claimTitleContainer, editClaimInput);
                                        
                                        // Add save/cancel buttons
                                        ui::Component saveClaimButton = ComponentGenerator::createButton(
                                            "saveClaimButton",
                                            "Save",
                                            "",
                                            "bg-green-600",
                                            "hover:bg-green-700",
                                            "text-white",
                                            "px-3 py-1",
                                            "rounded",
                                            "transition-colors text-sm ml-2"
                                        );
                                        ComponentGenerator::addChild(claimTitleContainer, saveClaimButton);
                                        
                                        ui::Component cancelEditClaimButton = ComponentGenerator::createButton(
                                            "cancelEditClaimButton",
                                            "Cancel",
                                            "",
                                            "bg-gray-600",
                                            "hover:bg-gray-700",
                                            "text-white",
                                            "px-3 py-1",
                                            "rounded",
                                            "transition-colors text-sm ml-2"
                                        );
                                        ComponentGenerator::addChild(claimTitleContainer, cancelEditClaimButton);
                                    } else {
                                        // Show normal title text
                                        ui::Component currentClaimTitle = ComponentGenerator::createText(
                                            "currentClaimTitle",
                                            claim,
                                            "text-lg",
                                            "text-white",
                                            "font-semibold",
                                            ""
                                        );
                                        ComponentGenerator::addChild(claimTitleContainer, currentClaimTitle);
                                        
                                        // Add edit button if user owns the claim and is in modify mode
                                        if (currentUserId == currentClaimCreatorId && modifyingCurrentClaim) {
                                            ui::Component editClaimButton = ComponentGenerator::createButton(
                                                "editClaimButton",
                                                "Edit",
                                                "",
                                                "bg-yellow-600",
                                                "hover:bg-yellow-700",
                                                "text-white",
                                                "px-3 py-1",
                                                "rounded",
                                                "transition-colors text-sm ml-3"
                                            );
                                            ComponentGenerator::addChild(claimTitleContainer, editClaimButton);
                                        }
                                    }
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    // Find descriptionContent and update it (or replace with input if editing)
    for (int i = 0; i < mainLayout.children_size(); i++) {
        ui::Component* contentArea = mainLayout.mutable_children(i);
        if (contentArea->id() == "contentArea") {
            for (int j = 0; j < contentArea->children_size(); j++) {
                ui::Component* centerContent = contentArea->mutable_children(j);
                if (centerContent->id() == "centerContent") {
                    for (int k = 0; k < centerContent->children_size(); k++) {
                        ui::Component* descriptionBox = centerContent->mutable_children(k);
                        if (descriptionBox->id() == "descriptionBox") {
                            // Find and update/replace description content
                            for (int l = 0; l < descriptionBox->children_size(); l++) {
                                ui::Component* child = descriptionBox->mutable_children(l);
                                if (child->id() == "descriptionContent") {
                                    if (editingDescription) {
                                        // Replace text with input field
                                        child->Clear();
                                        *child = ComponentGenerator::createInput(
                                            "editDescriptionInput",
                                            currentClaimDescription,
                                            "description",
                                            "bg-gray-700",
                                            "text-white",
                                            "border-gray-600",
                                            "p-3",
                                            "rounded",
                                            "w-full mb-6 h-32"
                                        );
                                    } else {
                                        // Just set the text
                                        child->set_text(currentClaimDescription);
                                    }
                                    break;
                                }
                            }
                            
                            // Update description actions based on editing state
                            for (int l = 0; l < descriptionBox->children_size(); l++) {
                                ui::Component* descriptionActions = descriptionBox->mutable_children(l);
                                if (descriptionActions->id() == "descriptionActions") {
                                    descriptionActions->clear_children();
                                    
                                    if (editingDescription) {
                                        // Show cancel and save buttons
                                        ui::Component cancelEditButton = ComponentGenerator::createButton(
                                            "cancelEditDescriptionButton",
                                            "Cancel",
                                            "",
                                            "bg-gray-600",
                                            "hover:bg-gray-700",
                                            "text-white",
                                            "px-4 py-2",
                                            "rounded",
                                            "transition-colors text-sm"
                                        );
                                        ComponentGenerator::addChild(descriptionActions, cancelEditButton);

                                        ui::Component saveDescriptionButton = ComponentGenerator::createButton(
                                            "saveDescriptionButton",
                                            "Save",
                                            "",
                                            "bg-green-600",
                                            "hover:bg-green-700",
                                            "text-white",
                                            "px-4 py-2",
                                            "rounded",
                                            "transition-colors text-sm"
                                        );
                                        ComponentGenerator::addChild(descriptionActions, saveDescriptionButton);
                                    }
                                    // Otherwise buttons are added by AddAppropriateButtons
                                    break;
                                }
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    return mainLayout;
}
ui::Component DebatePageGenerator::AddAppropriateButtons(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout){
    // Extract data
    int currentUserId = user.user_id();
    user_engagement::DebatingInfo debatingInfo = user.engagement().debating_info();
    int currentClaimCreatorId = debatingInfo.current_claim().creator_id();
    bool isChallenge = info.is_challenge_debate();
    bool modifyingCurrentClaim = debatingInfo.modifying_current_claim();
    
    // Check if we're in editing mode - if so, buttons are already added by FillCurrentClaimSection
    bool editingDescription = (debatingInfo.current_debate_action().action_type() == 
                               user_engagement::DebatingInfo_CurrentDebateAction_ActionType_EDITING_CLAIM_DESCRIPTION);
    bool editingClaim = (debatingInfo.current_debate_action().action_type() == 
                         user_engagement::DebatingInfo_CurrentDebateAction_ActionType_EDITING_CLAIM);
    
    // Check if user is challenging (action type is CHALLENGING_CLAIM)
    bool challengingClaim = (debatingInfo.current_debate_action().action_type() == 
                            user_engagement::DebatingInfo_CurrentDebateAction_ActionType_CHALLENGING_CLAIM);

    // Add Modify Claim buttons to rightContent
    for (int i = 0; i < mainLayout.children_size(); i++) {
        ui::Component* contentArea = mainLayout.mutable_children(i);
        if (contentArea->id() == "contentArea") {
            for (int j = 0; j < contentArea->children_size(); j++) {
                ui::Component* rightContent = contentArea->mutable_children(j);
                if (rightContent->id() == "rightContent") {
                    for (int k = 0; k < rightContent->children_size(); k++) {
                        ui::Component* modifyActionsContainer = rightContent->mutable_children(k);
                        if (modifyActionsContainer->id() == "modifyActionsContainer") {
                            if (modifyingCurrentClaim && !demo_mode::kReadOnlyMode) {
                                // Show Cancel and Submit buttons
                                ui::Component cancelModifyButton = ComponentGenerator::createButton(
                                    "cancelModifyClaimButton",
                                    "Cancel",
                                    "",
                                    "bg-gray-600",
                                    "hover:bg-gray-700",
                                    "text-white",
                                    "px-4 py-2",
                                    "rounded",
                                    "w-full transition-colors mb-2"
                                );
                                ComponentGenerator::addChild(modifyActionsContainer, cancelModifyButton);

                                ui::Component submitModifyButton = ComponentGenerator::createButton(
                                    "submitModifyClaimButton",
                                    "Submit",
                                    "",
                                    "bg-green-600",
                                    "hover:bg-green-700",
                                    "text-white",
                                    "px-4 py-2",
                                    "rounded",
                                    "w-full transition-colors"
                                );
                                ComponentGenerator::addChild(modifyActionsContainer, submitModifyButton);
                            } else if (!demo_mode::kReadOnlyMode) {
                                // Show Modify Claim button
                                ui::Component modifyClaimButton = ComponentGenerator::createButton(
                                    "modifyClaimButton",
                                    "Modify Claim",
                                    "",
                                    "bg-blue-600",
                                    "hover:bg-blue-700",
                                    "text-white",
                                    "px-4 py-2",
                                    "rounded",
                                    "w-full transition-colors"
                                );
                                ComponentGenerator::addChild(modifyActionsContainer, modifyClaimButton);
                            }
                            break;
                        }
                    }
                    break;
                }
            }
            break;
        }
    }

    // Only add buttons if not in editing mode
    if (!editingDescription && !editingClaim && !demo_mode::kReadOnlyMode) {
        // Find descriptionActions container and add appropriate buttons
        for (int i = 0; i < mainLayout.children_size(); i++) {
            ui::Component* contentArea = mainLayout.mutable_children(i);
            if (contentArea->id() == "contentArea") {
                for (int j = 0; j < contentArea->children_size(); j++) {
                    ui::Component* centerContent = contentArea->mutable_children(j);
                    if (centerContent->id() == "centerContent") {
                        for (int k = 0; k < centerContent->children_size(); k++) {
                            ui::Component* descriptionBox = centerContent->mutable_children(k);
                            if (descriptionBox->id() == "descriptionBox") {
                                for (int l = 0; l < descriptionBox->children_size(); l++) {
                                    ui::Component* descriptionActions = descriptionBox->mutable_children(l);
                                    if (descriptionActions->id() == "descriptionActions") {
                                        // Add buttons based on ownership
                                        if (currentUserId == currentClaimCreatorId && modifyingCurrentClaim) {
                                            // User owns the claim and is in modify mode
                                            ui::Component editDescriptionButton = ComponentGenerator::createButton(
                                                "editDescriptionButton",
                                                "Edit Description",
                                                "",
                                                "bg-yellow-600",
                                                "hover:bg-yellow-700",
                                                "text-white",
                                                "px-4 py-2",
                                                "rounded",
                                                "transition-colors text-sm"
                                            );
                                            ComponentGenerator::addChild(descriptionActions, editDescriptionButton);

                                            ui::Component addChildClaim = ComponentGenerator::createButton(
                                                "addChildClaimButton",
                                                "Add Child Claim",
                                                "",
                                                "bg-green-600",
                                                "hover:bg-green-700",
                                                "text-white",
                                                "px-4 py-2",
                                                "rounded",
                                                "transition-colors text-sm"
                                            );
                                            ComponentGenerator::addChild(descriptionActions, addChildClaim);
                                        } else if (currentUserId != currentClaimCreatorId) {
                                            // User does not own the claim
                                            if (challengingClaim) {
                                                // Currently challenging - show Cancel and Create Challenge buttons
                                                ui::Component cancelChallengeButton = ComponentGenerator::createButton(
                                                    "cancelChallengeButton",
                                                    "Cancel Challenge",
                                                    "",
                                                    "bg-gray-600",
                                                    "hover:bg-gray-700",
                                                    "text-white",
                                                    "px-4 py-2",
                                                    "rounded",
                                                    "transition-colors text-sm"
                                                );
                                                ComponentGenerator::addChild(descriptionActions, cancelChallengeButton);

                                                ui::Component openCreateChallengeButton = ComponentGenerator::createButton(
                                                    "openCreateChallengeButton",
                                                    "Create Challenge",
                                                    "",
                                                    "bg-green-600",
                                                    "hover:bg-green-700",
                                                    "text-white",
                                                    "px-4 py-2",
                                                    "rounded",
                                                    "transition-colors text-sm"
                                                );
                                                ComponentGenerator::addChild(descriptionActions, openCreateChallengeButton);
                                            } else {
                                                // Not challenging - show Challenge Claim button
                                                ui::Component challengeClaimButton = ComponentGenerator::createButton(
                                                    "challengeClaimButton",
                                                    "Challenge Claim",
                                                    "",
                                                    "bg-orange-600",
                                                    "hover:bg-orange-700",
                                                    "text-white",
                                                    "px-4 py-2",
                                                    "rounded",
                                                    "transition-colors text-sm"
                                                );
                                                ComponentGenerator::addChild(descriptionActions, challengeClaimButton);
                                            }
                                        }
                                        break;
                                    }
                                }
                                break;
                            }
                        }
                        break;
                    }
                }
                break;
            }
        }
    }

    // Add "Go to Challenged Claim" button if this is a challenge
    if (isChallenge) {
        for (int i = 0; i < mainLayout.children_size(); i++) {
            ui::Component* topSection = mainLayout.mutable_children(i);
            if (topSection->id() == "topSection") {
                for (int j = 0; j < topSection->children_size(); j++) {
                    ui::Component* leftTopSection = topSection->mutable_children(j);
                    if (leftTopSection->id() == "leftTopSection") {
                        // In challenge mode, replace default parent navigation with challenge-specific navigation.
                        leftTopSection->clear_children();
                        ui::Component goToChallengedClaimButton = ComponentGenerator::createButton(
                            "goToChallengedClaimButton",
                            "Go To Challenged Claim",
                            "",
                            "bg-orange-600",
                            "hover:bg-orange-700",
                            "text-white",
                            "px-4 py-2",
                            "rounded",
                            "transition-colors text-sm"
                        );
                        ComponentGenerator::addChild(leftTopSection, goToChallengedClaimButton);
                        break;
                    }
                }
                break;
            }
        }
    }

    return mainLayout;
}
ui::Component DebatePageGenerator::AddAppropriateOverlays(const rendering_info::DebatePageRenderingInfo& info, const user::User& user, ui::Component mainLayout){
    user_engagement::DebatingInfo debatingInfo = user.engagement().debating_info();
    
    switch(debatingInfo.current_debate_action().action_type()) {
        case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_ADDING_CHILD_CLAIM: {
            // Modal overlay
            ui::Component modalOverlay = ComponentGenerator::createContainer(
                "addChildClaimModalOverlay",
                "fixed inset-0 flex items-center justify-center",
                "bg-black/50",
                "",
                "",
                "",
                "",
                "z-50"
            );

            // Modal content
            ui::Component modalContent = ComponentGenerator::createContainer(
                "addChildClaimModalContent",
                "",
                "bg-gray-800",
                "p-8",
                "",
                "border-2 border-gray-700",
                "rounded-lg",
                "w-full max-w-2xl"
            );

            // Modal title
            ui::Component modalTitle = ComponentGenerator::createText(
                "addChildClaimModalTitle",
                "Add Child Claim",
                "text-2xl",
                "text-white",
                "font-bold",
                "mb-6"
            );
            ComponentGenerator::addChild(&modalContent, modalTitle);

            // Claim sentence label
            ui::Component claimSentenceLabel = ComponentGenerator::createText(
                "claimSentenceLabel",
                "Claim Sentence:",
                "text-sm",
                "text-white",
                "font-semibold",
                "mb-2"
            );
            ComponentGenerator::addChild(&modalContent, claimSentenceLabel);

            // Claim sentence input
            ui::Component claimSentenceInput = ComponentGenerator::createInput(
                "claimSentenceInput",
                "Enter the claim sentence...",
                "claimSentence",
                "bg-gray-700",
                "text-white",
                "border-gray-600",
                "p-3",
                "rounded",
                "w-full mb-4"
            );
            ComponentGenerator::addChild(&modalContent, claimSentenceInput);

            // Description label
            ui::Component descriptionLabel = ComponentGenerator::createText(
                "descriptionLabel",
                "Description:",
                "text-sm",
                "text-white",
                "font-semibold",
                "mb-2"
            );
            ComponentGenerator::addChild(&modalContent, descriptionLabel);

            // Description input (using input as textarea)
            ui::Component descriptionInput = ComponentGenerator::createInput(
                "descriptionInput",
                "Enter the description...",
                "description",
                "bg-gray-700",
                "text-white",
                "border-gray-600",
                "p-3",
                "rounded",
                "w-full mb-6 h-32"
            );
            ComponentGenerator::addChild(&modalContent, descriptionInput);

            // Modal action buttons
            ui::Component modalActions = ComponentGenerator::createContainer(
                "modalActions",
                "flex gap-3 justify-end",
                "",
                "",
                "",
                "",
                "",
                ""
            );

            ui::Component cancelButton = ComponentGenerator::createButton(
                "closeAddChildClaimButton",
                "Cancel",
                "",
                "bg-gray-600",
                "hover:bg-gray-700",
                "text-white",
                "px-6 py-2",
                "rounded",
                "transition-colors"
            );
            ComponentGenerator::addChild(&modalActions, cancelButton);

            ui::Component submitButton = ComponentGenerator::createButton(
                "submitAddChildClaimButton",
                "Submit",
                "",
                "bg-green-600",
                "hover:bg-green-700",
                "text-white",
                "px-6 py-2",
                "rounded",
                "transition-colors"
            );
            ComponentGenerator::addChild(&modalActions, submitButton);

            ComponentGenerator::addChild(&modalContent, modalActions);
            ComponentGenerator::addChild(&modalOverlay, modalContent);
            ComponentGenerator::addChild(&mainLayout, modalOverlay);
            break;
        }
        case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_CHALLENGING_CLAIM: {
            // Modal overlay
            // check if its actually opened
            bool isChallengeModalOpen = debatingInfo.challenging_info().opened_challenge_modal();
            if (!isChallengeModalOpen) {
                break;
            }

            ui::Component addChallengeModalOverlay = ComponentGenerator::createContainer(
                "addChallengeModalOverlay",
                "fixed inset-0 flex items-center justify-center",
                "bg-black/50",
                "",
                "",
                "",
                "",
                "z-50"
            );

            // Modal content
            ui::Component addChallengeModalContent = ComponentGenerator::createContainer(
                "addChallengeModalContent",
                "",
                "bg-gray-800",
                "p-8",
                "",
                "border-2 border-gray-700",
                "rounded-lg",
                "w-full max-w-2xl"
            );

            // Modal title
            ui::Component addChallengeModalTitle = ComponentGenerator::createText(
                "addChallengeModalTitle",
                "Challenge Sentence:",
                "text-2xl",
                "text-white",
                "font-bold",
                "mb-2"
            );
            ComponentGenerator::addChild(&addChallengeModalContent, addChallengeModalTitle);

            // Instructions text
            ui::Component challengeInstructions = ComponentGenerator::createText(
                "challengeInstructions",
                "Should contradict or prove the statement you are challenging wrong or faulty",
                "text-sm",
                "text-gray-400",
                "",
                "mb-6"
            );
            ComponentGenerator::addChild(&addChallengeModalContent, challengeInstructions);

            // Challenge sentence input
            ui::Component challengeSentenceInput = ComponentGenerator::createInput(
                "challengeSentenceInput",
                "Enter your challenge sentence...",
                "challengeSentence",
                "bg-gray-700",
                "text-white",
                "border-gray-600",
                "p-3",
                "rounded",
                "w-full mb-6 h-32"
            );
            ComponentGenerator::addChild(&addChallengeModalContent, challengeSentenceInput);

            // Modal action buttons
            ui::Component addChallengeModalActions = ComponentGenerator::createContainer(
                "addChallengeModalActions",
                "flex gap-3 justify-end",
                "",
                "",
                "",
                "",
                "",
                ""
            );

            ui::Component cancelChallengeModalButton = ComponentGenerator::createButton(
                "closeChallengeModalButton",
                "Cancel",
                "",
                "bg-gray-600",
                "hover:bg-gray-700",
                "text-white",
                "px-6 py-2",
                "rounded",
                "transition-colors"
            );
            ComponentGenerator::addChild(&addChallengeModalActions, cancelChallengeModalButton);

            ui::Component submitChallengeModalButton = ComponentGenerator::createButton(
                "submitChallengeModalButton",
                "Submit",
                "",
                "bg-green-600",
                "hover:bg-green-700",
                "text-white",
                "px-6 py-2",
                "rounded",
                "transition-colors"
            );
            ComponentGenerator::addChild(&addChallengeModalActions, submitChallengeModalButton);

            ComponentGenerator::addChild(&addChallengeModalContent, addChallengeModalActions);
            ComponentGenerator::addChild(&addChallengeModalOverlay, addChallengeModalContent);
            ComponentGenerator::addChild(&mainLayout, addChallengeModalOverlay);
            break;
        }
        case user_engagement::DebatingInfo_CurrentDebateAction_ActionType_CONNECTING_CLAIMS: {
            // Extract data for connecting claims
            // check if it's opened_connecting_modal
            int currentClaimId = debatingInfo.current_claim().id();
            std::string claim = debatingInfo.current_claim().sentence();
            int fromClaimId = debatingInfo.connecting_info().from_claim_id();
            int toClaimId = debatingInfo.connecting_info().to_claim_id();
            bool isConnectingModal = debatingInfo.connecting_info().opened_connect_modal();
            if (!isConnectingModal) {
                break;
            }
            
            std::string fromClaimSentence = "";
            std::string toClaimSentence = "";
            
            // Check if fromClaimId is the current claim
            if (fromClaimId == currentClaimId) {
                fromClaimSentence = claim;
            } else {
                // Search in child claims
                for (int i = 0; i < info.children_claims_size(); i++) {
                    const rendering_info::ClaimRenderInfo& childClaim = info.children_claims(i);
                    if (childClaim.id() == fromClaimId) {
                        fromClaimSentence = childClaim.sentence();
                        break;
                    }
                }
            }
            
            // Check if toClaimId is the current claim
            if (toClaimId == currentClaimId) {
                toClaimSentence = claim;
            } else {
                // Search in child claims
                for (int i = 0; i < info.children_claims_size(); i++) {
                    const rendering_info::ClaimRenderInfo& childClaim = info.children_claims(i);
                    if (childClaim.id() == toClaimId) {
                        toClaimSentence = childClaim.sentence();
                        break;
                    }
                }
            }

            // Modal overlay
            ui::Component connectModalOverlay = ComponentGenerator::createContainer(
                "connectModalOverlay",
                "fixed inset-0 flex items-center justify-center",
                "bg-black/50",
                "",
                "",
                "",
                "",
                "z-50"
            );

            // Modal content
            ui::Component connectModalContent = ComponentGenerator::createContainer(
                "connectModalContent",
                "",
                "bg-gray-800",
                "p-8",
                "",
                "border-2 border-gray-700",
                "rounded-lg",
                "w-full max-w-2xl"
            );

            // Modal title
            ui::Component connectModalTitle = ComponentGenerator::createText(
                "connectModalTitle",
                "Create Connection",
                "text-2xl",
                "text-white",
                "font-bold",
                "mb-6"
            );
            ComponentGenerator::addChild(&connectModalContent, connectModalTitle);

            // Display connecting claims
            ui::Component connectingClaimsContainer = ComponentGenerator::createContainer(
                "connectingClaimsContainer",
                "",
                "bg-gray-700",
                "p-4",
                "mb-6",
                "border border-gray-600",
                "rounded",
                ""
            );

            ui::Component fromClaimLabel = ComponentGenerator::createText(
                "fromClaimLabel",
                "From:",
                "text-xs",
                "text-gray-400",
                "font-semibold",
                "mb-1"
            );
            ComponentGenerator::addChild(&connectingClaimsContainer, fromClaimLabel);

            ui::Component fromClaimText = ComponentGenerator::createText(
                "fromClaimText",
                fromClaimSentence,
                "text-sm",
                "text-white",
                "",
                "mb-3"
            );
            ComponentGenerator::addChild(&connectingClaimsContainer, fromClaimText);

            ui::Component arrowText = ComponentGenerator::createText(
                "arrowText",
                "↓",
                "text-center text-2xl",
                "text-purple-400",
                "",
                "mb-3"
            );
            ComponentGenerator::addChild(&connectingClaimsContainer, arrowText);

            ui::Component toClaimLabel = ComponentGenerator::createText(
                "toClaimLabel",
                "To:",
                "text-xs",
                "text-gray-400",
                "font-semibold",
                "mb-1"
            );
            ComponentGenerator::addChild(&connectingClaimsContainer, toClaimLabel);

            ui::Component toClaimText = ComponentGenerator::createText(
                "toClaimText",
                toClaimSentence,
                "text-sm",
                "text-white",
                "",
                ""
            );
            ComponentGenerator::addChild(&connectingClaimsContainer, toClaimText);

            ComponentGenerator::addChild(&connectModalContent, connectingClaimsContainer);

            // Connection label
            ui::Component connectionLabel = ComponentGenerator::createText(
                "connectionLabel",
                "Connection:",
                "text-sm",
                "text-white",
                "font-semibold",
                "mb-2"
            );
            ComponentGenerator::addChild(&connectModalContent, connectionLabel);

            // Connection input
            ui::Component connectionInput = ComponentGenerator::createInput(
                "connectionInput",
                "Enter the connection type...",
                "connection",
                "bg-gray-700",
                "text-white",
                "border-gray-600",
                "p-3",
                "rounded",
                "w-full mb-6"
            );
            ComponentGenerator::addChild(&connectModalContent, connectionInput);

            // Modal action buttons
            ui::Component connectModalActions = ComponentGenerator::createContainer(
                "connectModalActions",
                "flex gap-3 justify-end",
                "",
                "",
                "",
                "",
                "",
                ""
            );

            ui::Component connectCancelButton = ComponentGenerator::createButton(
                "closeConnectModalButton",
                "Cancel",
                "",
                "bg-gray-600",
                "hover:bg-gray-700",
                "text-white",
                "px-6 py-2",
                "rounded",
                "transition-colors"
            );
            ComponentGenerator::addChild(&connectModalActions, connectCancelButton);

            ui::Component submitConnectClaimsButton = ComponentGenerator::createButton(
                "submitConnectClaimsButton",
                "Submit",
                "",
                "bg-green-600",
                "hover:bg-green-700",
                "text-white",
                "px-6 py-2",
                "rounded",
                "transition-colors"
            );
            ComponentGenerator::addChild(&connectModalActions, submitConnectClaimsButton);

            ComponentGenerator::addChild(&connectModalContent, connectModalActions);
            ComponentGenerator::addChild(&connectModalOverlay, connectModalContent);
            ComponentGenerator::addChild(&mainLayout, connectModalOverlay);
            break;
        }
        default:
            // No overlay needed
            break;
    }
    
    return mainLayout;
}

