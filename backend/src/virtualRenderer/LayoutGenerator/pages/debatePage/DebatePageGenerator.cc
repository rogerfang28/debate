#include "DebatePageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../utils/Log.h"

ui::Page DebatePageGenerator::GenerateDebatePage(user_engagement::UserEngagement engagement) {
    ui::Page page;
    page.set_page_id("debate");
    page.set_title("Debate View");
    int currentUserId = engagement.user_id();
    user_engagement::DebatingInfo debatingInfo = engagement.debating_info();
    bool isChallenge = debatingInfo.is_challenge();
    int debateId = debatingInfo.debate_id();
    int currentClaimId = debatingInfo.current_claim().id();
    int currentClaimCreatorId = debatingInfo.current_claim().creator_id();
    Log::debug("[DebatePageGenerator] Current User ID: " + std::to_string(currentUserId));
    Log::debug("[DebatePageGenerator] Current Claim ID: " + std::to_string(currentClaimId));
    Log::debug("[DebatePageGenerator] Current Claim Creator ID: " + std::to_string(currentClaimCreatorId));
    std::string debate_topic = debatingInfo.root_claim().sentence();
    std::string claim = debatingInfo.current_claim().sentence();
    std::vector<std::tuple<std::string,std::string,int>> childClaimInfo; // id, sentence, creator_id
    for (int i = 0; i < debatingInfo.children_claims_size(); i++) {
        const user_engagement::ClaimInfo& claim = debatingInfo.children_claims(i);
        childClaimInfo.push_back({std::to_string(claim.id()), claim.sentence(), claim.creator_id()});
    }
    bool openedAddChildClaimModal = debatingInfo.adding_child_claim();
    bool editingClaimDescription = debatingInfo.editing_claim_description();
    bool editingClaimSentence = debatingInfo.editing_claim_sentence();
    bool openedConnectModal = debatingInfo.connecting_info().opened_connect_modal();
    bool challengingClaim = debatingInfo.challenging_claim();
    bool openedAddChallengeModal = debatingInfo.challenging_info().opened_challenge_modal();
    std::vector<int> currentChallengedClaimIds;
    std::vector<int> currentChallengedLinkIds;
    // std::string currentChallengeSentence = debatingInfo.challenging_info().challenge_sentence();
    for (int i = 0; i < debatingInfo.challenging_info().claim_ids_size(); i++) {
        currentChallengedClaimIds.push_back(debatingInfo.challenging_info().claim_ids(i));
    }
    for (int i = 0; i < debatingInfo.challenging_info().link_ids_size(); i++) {
        currentChallengedLinkIds.push_back(debatingInfo.challenging_info().link_ids(i));
    }
    std::string currentClaimDescription = debatingInfo.current_claim_description();
    Log::debug("[DebatePageGenerator] Debate Topic: " + debate_topic);
    Log::debug("[DebatePageGenerator] Current Claim: " + claim);
    Log::debug("[DebatePageGenerator] Number of Child Claims: " + std::to_string(childClaimInfo.size()));
    
    std::vector<std::tuple<std::string, std::string, std::string, std::string>> linkInfo;
    for (int i = 0; i < debatingInfo.links_size(); i++) {
        const user_engagement::LinkInfo& link = debatingInfo.links(i);
        Log::debug("[DebatePageGenerator] Link from Claim ID: " + std::to_string(link.connect_from())
            + " to Claim ID: " + std::to_string(link.connect_to())
            + " with connection: " + link.connection());
        linkInfo.push_back({std::to_string(link.id()), std::to_string(link.connect_from()), std::to_string(link.connect_to()), link.connection()});
    }

    // connecting info
    bool connecting = debatingInfo.connecting_info().connecting();
    std::string fromClaimId = std::to_string(debatingInfo.connecting_info().from_claim_id());
    std::string toClaimId = std::to_string(debatingInfo.connecting_info().to_claim_id());
    Log::debug("[DebatePageGenerator] Connecting: " + std::to_string(connecting));
    Log::debug("[DebatePageGenerator] From Claim ID: " + fromClaimId
        + ", To Claim ID: " + toClaimId);

    std::vector<std::map<std::string, std::string>> challengesInfo;
    for (int i = 0; i < debatingInfo.current_challenges_size(); i++) {
        const user_engagement::ChallengeInfo& challenge = debatingInfo.current_challenges(i);
        challengesInfo.push_back({{"id", std::to_string(challenge.id())}, {"sentence", challenge.sentence()}, {"creator_id", std::to_string(challenge.creator_id())}});
    }
    // hardcode one for testing
    // challengesInfo.push_back({{"id", "1"}, {"sentence", "This is a challenge to the current claim."}, {"creator_id", "1"}});

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

    if (isChallenge) {
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
        ComponentGenerator::addChild(&leftTopSection, goToChallengedClaimButton);
    }

    ComponentGenerator::addChild(&topSection, leftTopSection);

    // Debate topic title in the center top
    // ui::Componen

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

    if (editingClaimSentence) {
        // Edit mode: show input field and buttons
        ui::Component claimInput = ComponentGenerator::createInput(
            "editClaimInput",
            "Enter the claim...",
            "claim",
            "bg-gray-700",
            "text-white",
            "border-gray-600",
            "p-3",
            "rounded",
            "w-full mb-3",
            claim
        );
        ComponentGenerator::addChild(&focusingOnClaim, claimInput);

        ui::Component claimEditActions = ComponentGenerator::createContainer(
            "claimEditActions",
            "flex gap-2 justify-center",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        ui::Component cancelEditClaimButton = ComponentGenerator::createButton(
            "cancelEditClaimButton",
            "Cancel",
            "",
            "bg-gray-600",
            "hover:bg-gray-700",
            "text-white",
            "px-4 py-1",
            "rounded",
            "transition-colors text-xs"
        );
        ComponentGenerator::addChild(&claimEditActions, cancelEditClaimButton);

        ui::Component saveClaimButton = ComponentGenerator::createButton(
            "saveClaimButton",
            "Save",
            "",
            "bg-green-600",
            "hover:bg-green-700",
            "text-white",
            "px-4 py-1",
            "rounded",
            "transition-colors text-xs"
        );
        ComponentGenerator::addChild(&claimEditActions, saveClaimButton);

        ComponentGenerator::addChild(&focusingOnClaim, claimEditActions);
    } else {
        // View mode: show text and edit button (only if user owns the claim)
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
            claim,
            "text-lg",
            "text-white",
            "font-semibold",
            ""
        );
        ComponentGenerator::addChild(&claimTitleContainer, currentClaimTitle);

        // Only show edit button if user owns the claim
        if (currentUserId == currentClaimCreatorId) {
            ui::Component editClaimButton = ComponentGenerator::createButton(
                "editClaimButton",
                "Edit",
                "",
                "bg-yellow-600",
                "hover:bg-yellow-700",
                "text-white",
                "px-3 py-1",
                "rounded",
                "transition-colors text-xs"
            );
            ComponentGenerator::addChild(&claimTitleContainer, editClaimButton);
        }

        ComponentGenerator::addChild(&focusingOnClaim, claimTitleContainer);
    }
    ComponentGenerator::addChild(&focusSection, focusingOnClaim);
    ComponentGenerator::addChild(&topSection, focusSection);

    // Right top section - minimap
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

    ui::Component minimap = ComponentGenerator::createContainer(
        "minimap",
        "",
        "bg-gray-800",
        "p-2",
        "",
        "border-2 border-gray-300",
        "rounded",
        "w-64 h-48"
    );

    ui::Component minimapTitle = ComponentGenerator::createText(
        "minimapTitle",
        "minimap",
        "text-xs",
        "text-gray-500",
        "",
        "mb-2"
    );
    ComponentGenerator::addChild(&minimap, minimapTitle);

    ui::Component minimapContent = ComponentGenerator::createContainer(
        "minimapContent",
        "grid grid-cols-4 gap-1 h-full",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    // Add minimap nodes
    for (int i = 1; i <= 8; i++) {
        std::string nodeId = "minimapNode" + std::to_string(i);
        std::string bgColor = (i == 1) ? "bg-yellow-200" : "bg-green-400";
        ui::Component minimapNode = ComponentGenerator::createContainer(
            nodeId,
            "",
            bgColor,
            "",
            "",
            "border",
            "rounded",
            ""
        );
        ComponentGenerator::addChild(&minimapContent, minimapNode);
    }

    ComponentGenerator::addChild(&minimap, minimapContent);
    // ComponentGenerator::addChild(&rightTopSection, minimap);
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

    // Left content - guidance box
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

    // Challenges section
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

    // Challenges container
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

    // Loop through challenges and create components
    for (const auto& challenge : challengesInfo) {
        std::string challengeId = challenge.at("id");
        std::string challengeSentence = challenge.at("sentence");
        int challengeCreatorId = std::stoi(challenge.at("creator_id"));
        bool userOwnsChallenge = (currentUserId == challengeCreatorId);
        
        ui::Component challengeNode = ComponentGenerator::createContainer(
            "challengeNode_" + challengeId,
            "",
            "bg-orange-600",
            "p-4",
            "",
            "border-2 border-orange-500",
            "rounded",
            ""
        );

        ui::Component challengeSentenceText = ComponentGenerator::createText(
            "challengeSentence_" + challengeId,
            challengeSentence,
            "",
            "text-white",
            "",
            "mb-3"
        );
        ComponentGenerator::addChild(&challengeNode, challengeSentenceText);

        // Button container for challenge actions
        ui::Component challengeButtonContainer = ComponentGenerator::createContainer(
            "challengeButtonContainer_" + challengeId,
            "flex gap-2",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        ui::Component viewChallengeButton = ComponentGenerator::createButton(
            "viewChallengeButton_" + challengeId,
            "View Challenge",
            "",
            "bg-orange-700",
            "hover:bg-orange-800",
            "text-white",
            "px-4 py-2",
            "rounded",
            "flex-1 transition-colors text-sm"
        );
        ComponentGenerator::addChild(&challengeButtonContainer, viewChallengeButton);

        // Only show delete button if user owns the challenge
        if (userOwnsChallenge) {
            ui::Component deleteChallengeButton = ComponentGenerator::createButton(
                "deleteChallengeButton_" + challengeId,
                "Delete",
                "",
                "bg-red-600",
                "hover:bg-red-700",
                "text-white",
                "px-4 py-2",
                "rounded",
                "transition-colors text-sm"
            );
            // ComponentGenerator::addChild(&challengeButtonContainer, deleteChallengeButton);
        }

        ComponentGenerator::addChild(&challengeNode, challengeButtonContainer);

        ComponentGenerator::addChild(&challengesContainer, challengeNode);
    }

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

    if (editingClaimDescription) {
        // Edit mode: show input field
        ui::Component descriptionInput = ComponentGenerator::createInput(
            "editDescriptionInput",
            "Enter the description...",
            "description",
            "bg-gray-700",
            "text-white",
            "border-gray-600",
            "p-3",
            "rounded",
            "w-full mb-6 h-32",
            currentClaimDescription
        );
        // Set the current description as the value (this would need to be handled in the input component)
        ComponentGenerator::addChild(&descriptionBox, descriptionInput);

        // Edit mode actions
        ui::Component descriptionActions = ComponentGenerator::createContainer(
            "descriptionActions",
            "flex gap-3",
            "",
            "",
            "",
            "",
            "",
            ""
        );

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
        ComponentGenerator::addChild(&descriptionActions, cancelEditButton);

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
        ComponentGenerator::addChild(&descriptionActions, saveDescriptionButton);

        ComponentGenerator::addChild(&descriptionBox, descriptionActions);
    } else {
        // View mode: show text
        ui::Component descriptionContent = ComponentGenerator::createText(
            "descriptionContent",
            currentClaimDescription,
            "",
            "text-white",
            "",
            "leading-relaxed mb-6"
        );
        ComponentGenerator::addChild(&descriptionBox, descriptionContent);

        // Description actions
        ui::Component descriptionActions = ComponentGenerator::createContainer(
            "descriptionActions",
            "flex gap-3",
            "",
            "",
            "",
            "",
            "",
            ""
        );

        // Check if user owns the current claim
        if (currentUserId == currentClaimCreatorId) {
            // User owns the claim - show edit, add, delete buttons
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
            ComponentGenerator::addChild(&descriptionActions, editDescriptionButton);

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
            ComponentGenerator::addChild(&descriptionActions, addChildClaim);

            ui::Component deleteStatementButton = ComponentGenerator::createButton(
                "deleteStatementButton",
                "Delete Statement",
                "",
                "bg-red-600",
                "hover:bg-red-700",
                "text-white",
                "px-4 py-2",
                "rounded",
                "transition-colors text-sm"
            );
            // ComponentGenerator::addChild(&descriptionActions, deleteStatementButton); // removed for now
        } else {
            // User does NOT own the claim - show Challenge Claim or Cancel/Submit Challenge buttons
            if (challengingClaim) {
                // Currently challenging - show Cancel and Submit Challenge buttons
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
                ComponentGenerator::addChild(&descriptionActions, cancelChallengeButton);

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
                ComponentGenerator::addChild(&descriptionActions, openCreateChallengeButton);
            } else {
                // Not challenging - show Challenge Claim button in orange
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
                ComponentGenerator::addChild(&descriptionActions, challengeClaimButton);
            }
        }

        ComponentGenerator::addChild(&descriptionBox, descriptionActions);
    }
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
        ""
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
        "grid grid-cols-1 md:grid-cols-2 lg:grid-cols-3 gap-4",
        "",
        "",
        "",
        "",
        "",
        ""
    );

    // Use actual child claims from the vector
    for (size_t i = 0; i < childClaimInfo.size(); i++) {
        std::string claimId = std::get<0>(childClaimInfo[i]);
        std::string claimSentence = std::get<1>(childClaimInfo[i]);
        int claimCreatorId = std::get<2>(childClaimInfo[i]);
        bool userOwnsChildClaim = (currentUserId == claimCreatorId);
        
        std::string nodeId = "childNode_" + claimId;
        ui::Component childNode = ComponentGenerator::createContainer(
            nodeId,
            "",
            "bg-gray-700",
            "p-4",
            "",
            "border border-gray-700",
            "rounded",
            ""
        );

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
                    "flex gap-1",
                    "",
                    "",
                    "",
                    "",
                    "",
                    ""
                );

                // Delete link button - only show if user owns the child claim (the "from" claim)
                if (userOwnsChildClaim) {
                    ui::Component deleteLinkButton = ComponentGenerator::createButton(
                        "deleteLinkButton_" + linkId,
                        "Delete",
                        "",
                        "bg-red-600",
                        "hover:bg-red-700",
                        "text-white",
                        "px-2 py-1",
                        "rounded",
                        "transition-colors text-xs"
                    );
                    ComponentGenerator::addChild(&linkButtonContainer, deleteLinkButton);
                }

                // Challenge buttons - show if user is challenging claim
                if (challengingClaim) {
                    // Check if this link is in the current challenge
                    bool linkInChallenge = std::find(currentChallengedLinkIds.begin(), 
                                                     currentChallengedLinkIds.end(), 
                                                     std::stoi(linkId)) != currentChallengedLinkIds.end();
                    
                    if (linkInChallenge) {
                        ui::Component removeLinkFromChallengeButton = ComponentGenerator::createButton(
                            "removeLinkFromChallengeButton_" + linkId,
                            "Remove from Challenge",
                            "",
                            "bg-orange-600",
                            "hover:bg-orange-700",
                            "text-white",
                            "px-2 py-1",
                            "rounded",
                            "transition-colors text-xs"
                        );
                        ComponentGenerator::addChild(&linkButtonContainer, removeLinkFromChallengeButton);
                    } else {
                        ui::Component addLinkToChallengeButton = ComponentGenerator::createButton(
                            "addLinkToChallengeButton_" + linkId,
                            "Add to Challenge",
                            "",
                            "bg-orange-600",
                            "hover:bg-orange-700",
                            "text-white",
                            "px-2 py-1",
                            "rounded",
                            "transition-colors text-xs"
                        );
                        ComponentGenerator::addChild(&linkButtonContainer, addLinkToChallengeButton);
                    }
                }

                ComponentGenerator::addChild(&linkContainer, linkButtonContainer);
                ComponentGenerator::addChild(&childNode, linkContainer);
            }
        }

        ui::Component childNodeButtonContainer = ComponentGenerator::createContainer(
            nodeId + "ButtonContainer",
            "flex gap-2",
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
            "flex-1 transition-colors text-sm"
        );
        ComponentGenerator::addChild(&childNodeButtonContainer, childNodeButton);

        // Challenge buttons - show if user is challenging claim
        if (challengingClaim) {
            // Check if this claim is in the current challenge
            bool claimInChallenge = std::find(currentChallengedClaimIds.begin(), 
                                             currentChallengedClaimIds.end(), 
                                             std::stoi(claimId)) != currentChallengedClaimIds.end();
            
            if (claimInChallenge) {
                ui::Component removeClaimFromChallengeButton = ComponentGenerator::createButton(
                    "removeClaimFromChallengeButton_" + claimId,
                    "Remove from Challenge",
                    "",
                    "bg-orange-600",
                    "hover:bg-orange-700",
                    "text-white",
                    "px-4 py-2",
                    "rounded",
                    "transition-colors text-sm"
                );
                ComponentGenerator::addChild(&childNodeButtonContainer, removeClaimFromChallengeButton);
            } else {
                ui::Component addClaimToChallengeButton = ComponentGenerator::createButton(
                    "addClaimToChallengeButton_" + claimId,
                    "Add to Challenge",
                    "",
                    "bg-orange-600",
                    "hover:bg-orange-700",
                    "text-white",
                    "px-4 py-2",
                    "rounded",
                    "transition-colors text-sm"
                );
                ComponentGenerator::addChild(&childNodeButtonContainer, addClaimToChallengeButton);
            }
        }

        // Only show connection and delete buttons if user owns the claim
        if (userOwnsChildClaim) {
            // Add connection buttons based on connecting state
            if (connecting) {
                // If we're connecting and this is the FROM claim, show Cancel button
                if (claimId == fromClaimId) {
                    ui::Component cancelConnectClaimsButton = ComponentGenerator::createButton(
                        "cancelConnectClaimsButton_" + claimId,
                        "Cancel Connection",
                        "",
                        "bg-gray-600",
                        "hover:bg-gray-700",
                        "text-white",
                        "px-4 py-2",
                        "rounded",
                        "transition-colors text-sm"
                    );
                    ComponentGenerator::addChild(&childNodeButtonContainer, cancelConnectClaimsButton);
                } else {
                    // For all other claims, show Connect To button
                    ui::Component connectToClaimButton = ComponentGenerator::createButton(
                        "connectToClaimButton_" + claimId,
                        "Connect To",
                        "",
                        "bg-purple-600",
                        "hover:bg-purple-700",
                        "text-white",
                        "px-4 py-2",
                        "rounded",
                        "transition-colors text-sm"
                    );
                    ComponentGenerator::addChild(&childNodeButtonContainer, connectToClaimButton);
                }
            } else {
                // If we're not connecting, show Connect From button
                ui::Component connectFromClaimButton = ComponentGenerator::createButton(
                    "connectFromClaimButton_" + claimId,
                    "Connect From",
                    "",
                    "bg-purple-600",
                    "hover:bg-purple-700",
                    "text-white",
                    "px-4 py-2",
                    "rounded",
                    "transition-colors text-sm"
                );
                ComponentGenerator::addChild(&childNodeButtonContainer, connectFromClaimButton);
            }

            ui::Component deleteChildClaimButton = ComponentGenerator::createButton(
                "deleteChildClaimButton_" + claimId,
                "Delete",
                "",
                "bg-red-600",
                "hover:bg-red-700",
                "text-white",
                "px-4 py-2",
                "rounded",
                "transition-colors text-sm"
            );
            ComponentGenerator::addChild(&childNodeButtonContainer, deleteChildClaimButton);
        }

        ComponentGenerator::addChild(&childNode, childNodeButtonContainer);
        ComponentGenerator::addChild(&childArgumentsGrid, childNode);
    }

    ComponentGenerator::addChild(&childArgumentsSection, childArgumentsGrid);
    ComponentGenerator::addChild(&centerContent, childArgumentsSection);
    ComponentGenerator::addChild(&contentArea, centerContent);

    // Right content - guide and report section
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

    // Guide section
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

    // Report section
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
    ComponentGenerator::addChild(&contentArea, rightContent);
    ComponentGenerator::addChild(&mainLayout, contentArea);

    // Add Child Claim Modal
    if (openedAddChildClaimModal) {
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
    }

    // Connect Modal
    if (openedConnectModal) {
        // Find the sentence for fromClaimId and toClaimId
        std::string fromClaimSentence = "";
        std::string toClaimSentence = "";
        
        // Check if fromClaimId is the current claim
        if (fromClaimId == std::to_string(currentClaimId)) {
            fromClaimSentence = claim;
        } else {
            // Search in child claims
            for (const auto& childClaim : childClaimInfo) {
                    if (std::get<0>(childClaim) == fromClaimId) {
                        fromClaimSentence = std::get<1>(childClaim);
                    break;
                }
            }
        }
        
        // Check if toClaimId is the current claim
        if (toClaimId == std::to_string(currentClaimId)) {
            toClaimSentence = claim;
        } else {
            // Search in child claims
            for (const auto& childClaim : childClaimInfo) {
                if (std::get<0>(childClaim) == toClaimId) {
                    toClaimSentence = std::get<1>(childClaim);
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
    }

    // Add Challenge Modal
    if (openedAddChallengeModal) {
        // Modal overlay
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
    }

    // Add main layout to page
    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(mainLayout);

    return page;
}


