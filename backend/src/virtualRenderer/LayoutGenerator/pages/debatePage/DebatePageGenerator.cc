#include "DebatePageGenerator.h"
#include "../../../LayoutGenerator/ComponentGenerator.h"
#include "../../../../utils/Log.h"

ui::Page DebatePageGenerator::GenerateDebatePage(user_engagement::DebatingInfo debatingInfo) {
    ui::Page page;
    page.set_page_id("debate");
    page.set_title("Debate View");

    std::string currentClaimId = debatingInfo.current_claim().id();
    std::string debate_topic = debatingInfo.root_claim().sentence();
    std::string claim = debatingInfo.current_claim().sentence();
    std::vector<std::pair<std::string,std::string>> childClaimInfo;
    for (int i = 0; i < debatingInfo.children_claims_size(); i++) {
        const user_engagement::ClaimInfo& claim = debatingInfo.children_claims(i);
        childClaimInfo.push_back({claim.id(), claim.sentence()});
    }
    bool openedAddChildClaimModal = debatingInfo.adding_child_claim();
    bool editingClaimDescription = debatingInfo.editing_claim_description();
    bool editingClaimSentence = debatingInfo.editing_claim_sentence();
    bool openedConnectModal = debatingInfo.connecting_info().opened_connect_modal();
    std::string currentClaimDescription = debatingInfo.current_claim_description();
    Log::debug("[DebatePageGenerator] Debate Topic: " + debate_topic);
    Log::debug("[DebatePageGenerator] Current Claim: " + claim);
    Log::debug("[DebatePageGenerator] Number of Child Claims: " + std::to_string(childClaimInfo.size()));

    // connecting info
    bool connecting = debatingInfo.connecting_info().connecting();
    std::string fromClaimId = debatingInfo.connecting_info().from_claim_id();
    std::string toClaimId = debatingInfo.connecting_info().to_claim_id();
    Log::debug("[DebatePageGenerator] Connecting: " + std::to_string(connecting));
    Log::debug("[DebatePageGenerator] From Claim ID: " + fromClaimId
        + ", To Claim ID: " + toClaimId);

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
        // View mode: show text and edit button
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
    ComponentGenerator::addChild(&rightTopSection, minimap);
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

    ui::Component guidanceBox = ComponentGenerator::createContainer(
        "guidanceBox",
        "",
        "bg-gray-800",
        "p-6",
        "",
        "border-2 border-gray-700",
        "rounded",
        "min-h-64"
    );

    ui::Component guidanceText = ComponentGenerator::createText(
        "guidanceText",
        "Guide: ",
        "",
        "text-white",
        "",
        ""
    );
    ComponentGenerator::addChild(&guidanceBox, guidanceText);
    ComponentGenerator::addChild(&leftContent, guidanceBox);
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
        "Child Arguments:",
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
        std::string nodeId = "childNode_" + childClaimInfo[i].first;
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
            childClaimInfo[i].second,
            "",
            "text-white",
            "",
            "mb-3"
        );
        ComponentGenerator::addChild(&childNode, childNodeTitle);

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
            "viewChildNodeButton_" + childClaimInfo[i].first,
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

        // Add connection buttons based on connecting state
        if (connecting) {
            // If we're connecting and this is the FROM claim, show Cancel button
            if (childClaimInfo[i].first == fromClaimId) {
                ui::Component cancelConnectClaimsButton = ComponentGenerator::createButton(
                    "cancelConnectClaimsButton_" + childClaimInfo[i].first,
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
                    "connectToClaimButton_" + childClaimInfo[i].first,
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
                "connectFromClaimButton_" + childClaimInfo[i].first,
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
            "deleteChildClaimButton_" + childClaimInfo[i].first,
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

        ComponentGenerator::addChild(&childNode, childNodeButtonContainer);
        ComponentGenerator::addChild(&childArgumentsGrid, childNode);
    }

    ComponentGenerator::addChild(&childArgumentsSection, childArgumentsGrid);
    ComponentGenerator::addChild(&centerContent, childArgumentsSection);
    ComponentGenerator::addChild(&contentArea, centerContent);

    // Right content - report section
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

        ui::Component connectSubmitButton = ComponentGenerator::createButton(
            "submitConnectButton",
            "Submit",
            "",
            "bg-green-600",
            "hover:bg-green-700",
            "text-white",
            "px-6 py-2",
            "rounded",
            "transition-colors"
        );
        ComponentGenerator::addChild(&connectModalActions, connectSubmitButton);

        ComponentGenerator::addChild(&connectModalContent, connectModalActions);
        ComponentGenerator::addChild(&connectModalOverlay, connectModalContent);
        ComponentGenerator::addChild(&mainLayout, connectModalOverlay);
    }

    // Add main layout to page
    ui::Component* pageLayout = page.add_components();
    pageLayout->CopyFrom(mainLayout);

    return page;
}


