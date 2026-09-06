#include "../../../../../utils/DemoMode.h"
#include "StepView.h"
#include "../../../ComponentGenerator.h"
#include "../FullDebateView/FullDebatePageGenerator.h"
#include "../FullDebateView/FullDebatePageInfoParser.h"
#include "../../../../utils/UserNameResolver.h"
#include "../../../../../utils/TutorialLibrary.h"
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
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

// Local copy of FullDebatePageInfoParser.cc's MapClaimStatusForUser -- that
// one isn't declared in a header either. The Claim.status field itself is
// not the source of truth; status is per-viewer (an explicit user_statuses
// entry, else TRUE_CLAIM if the viewer created it, else UNDETERMINED).
static debate::ClaimStatus StepViewMapClaimStatusForUser(
    const debate::Claim& claim,
    int viewerUserId,
    const std::string& viewerUsername
) {
    const auto& userStatuses = claim.user_statuses();
    auto it = userStatuses.find(viewerUsername);
    if (it != userStatuses.end()) {
        return it->second;
    }
    if (viewerUserId == claim.creator_id()) {
        return debate::ClaimStatus::TRUE_CLAIM;
    }
    return debate::ClaimStatus::UNDETERMINED;
}

// Builds the "Selected Claim" info panel: sentence, status, creator,
// description, and contextual action buttons (Support + Delete for the
// viewer's own claims, Challenge for others').
static ui::Component BuildSelectedClaimPanel(
    int claimId,
    const rendering_info::FullDebateViewInfo& fullDebateInfo,
    const debate::Collection& collectionProto,
    VRUserDatabase& userDb,
    int viewerUserId,
    const std::string& viewerUsername
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
    debate::ClaimStatus viewerStatus = StepViewMapClaimStatusForUser(claim, viewerUserId, viewerUsername);
    addField("Status", "Status:", StepViewClaimStatusToLabel(viewerStatus), false);
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

    // A "Concede" button shows when the selected claim is itself a challenge
    // claim (from_claim_id == claimId, is_challenge) and the claim it
    // challenges was created by the viewer -- i.e. someone is challenging
    // the viewer's own claim, and the viewer is looking at that challenge.
    int concedeChallengeLinkId = -1;
    for (const auto& link : fullDebateInfo.full_debate_tree().links()) {
        if (link.is_challenge() && link.from_claim_id() == claimId) {
            auto challengedIt = collectionProto.claims_by_id().find(link.to_claim_id());
            if (challengedIt != collectionProto.claims_by_id().end() &&
                viewerUserId > 0 && challengedIt->second.creator_id() == viewerUserId) {
                concedeChallengeLinkId = link.link_id();
            }
            break;
        }
    }

    const bool isOwnClaim = viewerUserId > 0 && claim.creator_id() == viewerUserId;
    if (isOwnClaim) {
        // "Support", not "Add Child": a child claim is evidence offered FOR its
        // parent, so the relationship is what the user is actually creating.
        // The component id stays addChildClaimButton -- it is the wire to the
        // existing event handler, not a label.
        ui::Component addChildButton = ComponentGenerator::createButton(
            "addChildClaimButton",
            "Support",
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
        // challengeClaimButton now enters challenging mode AND opens the
        // challenge modal in one step (see ChallengeHandler::StartChallengeClaim).
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

    if (concedeChallengeLinkId > 0) {
        ui::Component concedeButton = ComponentGenerator::createButton(
            "concedeChallengeButton_" + std::to_string(concedeChallengeLinkId),
            "Concede",
            "",
            "bg-red-600",
            "hover:bg-red-700",
            "text-white",
            "px-3 py-1.5",
            "rounded",
            "text-sm"
        );
        ComponentGenerator::addChild(&buttonRow, concedeButton);
    }
    ComponentGenerator::addChild(&panel, buttonRow);

    return panel;
}

// Builds the "Current Status of Debate" panel. Keeps it simple: purely about
// unanswered challenges.
//   - No live (unanswered) challenge anywhere  -> "No current challenges."
//   - A live challenge targets one of the viewer's claims -> the viewer must
//     respond (list those challenge claims).
//   - A live challenge exists but none target the viewer -> "Waiting..." (it's
//     the opponent's move).
// A challenge Ck -> T counts as "answered" if it was challenged back (some
// challenge link points to Ck) or conceded (T's owner marked Ck TRUE_CLAIM,
// which is exactly what ConcedeChallenge writes).
// The panel for a selected ARGUMENT: the relation between claims, the pill in
// the graph. It takes the claim panel's place -- same id, same shape -- so the
// column shows one thing at a time. Everything here is read from the
// collection, which carries each relation with its from-set, its target, its
// basis, and any challenge aimed at the relation itself.
static ui::Component BuildSelectedRelationPanel(
    int relationId,
    const debate::Collection& collectionProto,
    VRUserDatabase& userDb,
    int viewerUserId,
    const std::string& viewerUsername
) {
    ui::Component panel = ComponentGenerator::createContainer(
        "selectedClaimPanel",
        "w-full",
        "bg-gray-800",
        "p-4",
        "",
        "border border-orange-700",
        "rounded-lg",
        ""
    );
    (*panel.mutable_css())["display"] = "flex";
    (*panel.mutable_css())["flex-direction"] = "column";
    (*panel.mutable_css())["gap"] = "0.5rem";

    auto relIt = collectionProto.links_by_id().find(relationId);
    if (relIt == collectionProto.links_by_id().end()) {
        ComponentGenerator::addChild(&panel, ComponentGenerator::createText(
            "selectedRelationEmpty", "No argument selected.", "text-sm", "text-gray-400", "", ""));
        return panel;
    }
    const debate::Relationship& rel = relIt->second;

    auto sentenceOf = [&](int claimId) {
        auto it = collectionProto.claims_by_id().find(claimId);
        return it != collectionProto.claims_by_id().end()
            ? it->second.sentence() : "claim " + std::to_string(claimId);
    };
    auto addField = [&](const std::string& idSuffix, const std::string& label,
                        const std::string& value, const std::string& valueColor) {
        ui::Component row = ComponentGenerator::createContainer(
            "selectedRelation" + idSuffix + "Row", "", "", "", "", "", "", "");
        (*row.mutable_css())["display"] = "flex";
        (*row.mutable_css())["flex-direction"] = "column";
        (*row.mutable_css())["gap"] = "0.15rem";
        ComponentGenerator::addChild(&row, ComponentGenerator::createText(
            "selectedRelation" + idSuffix + "Label", label, "text-xs", "text-gray-500",
            "font-semibold uppercase tracking-wide", ""));
        ui::Component valueText = ComponentGenerator::createText(
            "selectedRelation" + idSuffix + "Value", value, "text-sm", valueColor, "", "");
        (*valueText.mutable_css())["overflow-wrap"] = "break-word";
        ComponentGenerator::addChild(&row, valueText);
        ComponentGenerator::addChild(&panel, row);
    };

    const bool opposes = rel.role() == debate::OPPOSES;

    // The target: a claim, or -- for an undercut -- another argument. Either
    // way there is a claim to go back to.
    std::string targetText;
    int backToClaimId = 0;
    if (rel.to_case() == debate::Relationship::kToClaimId) {
        targetText = sentenceOf(rel.to_claim_id());
        backToClaimId = rel.to_claim_id();
    } else {
        targetText = "another argument (the one for: " + sentenceOf(rel.link().connect_to()) + ")";
        backToClaimId = rel.link().connect_to();
    }
    addField("Target", opposes ? "Selected Argument Against:" : "Selected Argument For:",
             targetText, "text-white");

    std::string sources;
    for (int from : rel.from_claim_ids()) {
        if (!sources.empty()) {
            sources += "  +  ";
        }
        sources += sentenceOf(from);
    }
    addField("Sources", rel.from_claim_ids_size() > 1 ? "Offered Together:" : "Offered:",
             sources, "text-white");

    std::string basis = rel.basis();
    if (basis.empty() || basis == "parent to child connection" || basis == "challenge link") {
        basis = "(no kind of reason chosen)";
    }
    addField("Basis", "Kind of Reason:", basis, "text-white");

    // Standing FOR THE VIEWER. An argument is undercut for whoever holds a
    // challenge against it TRUE; for them it carries nothing. Others' attacks
    // are shown as a count, since they change nothing for this viewer.
    int attacks = 0;
    bool undercut = false;
    for (const auto& entry : collectionProto.links_by_id()) {
        const debate::Relationship& other = entry.second;
        if (other.to_case() != debate::Relationship::kToRelationId ||
            other.to_relation_id() != relationId) {
            continue;
        }
        ++attacks;
        for (int from : other.from_claim_ids()) {
            auto cIt = collectionProto.claims_by_id().find(from);
            if (cIt != collectionProto.claims_by_id().end() &&
                StepViewMapClaimStatusForUser(cIt->second, viewerUserId, viewerUsername) ==
                    debate::ClaimStatus::TRUE_CLAIM) {
                undercut = true;
            }
        }
    }
    if (undercut) {
        addField("Standing", "Standing For You:",
                 "Undercut -- you hold a challenge against this argument, so it carries nothing for you.",
                 "text-red-300");
    } else if (attacks > 0) {
        addField("Standing", "Standing For You:",
                 "Challenged by " + std::to_string(attacks) + " argument(s) you do not hold; still counts for you.",
                 "text-yellow-300");
    } else {
        addField("Standing", "Standing For You:", "Standing -- nobody has challenged this argument.",
                 "text-green-300");
    }

    std::string creatorUsername = userDb.getUsername(rel.creator_id());
    if (creatorUsername.empty()) {
        creatorUsername = "User " + std::to_string(rel.creator_id());
    }
    addField("Creator", "Offered by:", creatorUsername, "text-white");

    ui::Component buttonRow = ComponentGenerator::createContainer(
        "selectedRelationButtonRow", "flex gap-2", "", "", "", "", "", "");
    (*buttonRow.mutable_css())["margin-top"] = "0.5rem";
    if (!demo_mode::kReadOnlyMode) {
        // One click: opens the challenge modal with this argument picked.
        ComponentGenerator::addChild(&buttonRow, ComponentGenerator::createButton(
            "challengeSelectedLinkButton_" + std::to_string(relationId),
            "Challenge this argument", "", "bg-orange-600", "hover:bg-orange-700",
            "text-white", "px-3 py-1.5", "rounded", "text-sm"));
    }
    if (backToClaimId > 0) {
        ComponentGenerator::addChild(&buttonRow, ComponentGenerator::createButton(
            "selectClaimNode_" + std::to_string(backToClaimId),
            "Back to the claim", "", "bg-gray-600", "hover:bg-gray-500",
            "text-white", "px-3 py-1.5", "rounded", "text-sm"));
    }
    ComponentGenerator::addChild(&panel, buttonRow);
    return panel;
}

static ui::Component BuildDebateStatusPanel(
    const rendering_info::FullDebateViewInfo& fullDebateInfo,
    const debate::Collection& collectionProto,
    VRUserDatabase& userDb,
    int viewerUserId,
    const std::string& viewerUsername
) {
    ui::Component panel = ComponentGenerator::createContainer(
        "debateStatusPanel",
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

    ui::Component heading = ComponentGenerator::createText(
        "debateStatusHeading",
        "Current Status of Debate:",
        "text-xs",
        "text-gray-500",
        "font-semibold uppercase tracking-wide",
        ""
    );
    ComponentGenerator::addChild(&panel, heading);

    std::vector<int> myChallengesToRespond;  // live challenges targeting the viewer's claims
    std::vector<int> myPendingChallenges;    // live challenges the viewer made, awaiting a response
    bool anyLiveChallenge = false;

    if (fullDebateInfo.has_full_debate_tree()) {
        const auto& tree = fullDebateInfo.full_debate_tree();

        // PARENT_CHILD children of each claim, e.g. an evidence claim added
        // under a challenge -- needed below to look through evidence when
        // deciding whether a challenge has been "answered".
        std::unordered_map<int, std::vector<int>> childrenOf;
        for (const auto& link : tree.links()) {
            if (!link.is_challenge()) {
                childrenOf[link.from_claim_id()].push_back(link.to_claim_id());
            }
        }

        // Any claim that has itself been challenged back has been "answered"
        // and is no longer live.
        std::unordered_set<int> challengedBack;
        for (const auto& link : tree.links()) {
            if (link.is_challenge()) {
                challengedBack.insert(link.to_claim_id());
            }
        }

        // A challenge is "answered" if IT, or any PARENT_CHILD descendant of
        // it (e.g. an evidence claim attached to it), was itself challenged
        // back or conceded. Challenges often target a claim's evidence
        // rather than the claim itself, so the direct id alone isn't enough.
        auto isAnswered = [&](int challengeClaimId, const std::string& targetOwnerName) {
            std::vector<int> stack = {challengeClaimId};
            std::unordered_set<int> visited;
            while (!stack.empty()) {
                int id = stack.back();
                stack.pop_back();
                if (!visited.insert(id).second) continue;

                if (challengedBack.count(id)) return true;

                auto it = collectionProto.claims_by_id().find(id);
                if (it != collectionProto.claims_by_id().end()) {
                    const auto& statuses = it->second.user_statuses();
                    auto statusIt = statuses.find(targetOwnerName);
                    if (statusIt != statuses.end() && statusIt->second == debate::ClaimStatus::TRUE_CLAIM) {
                        return true;  // conceded
                    }
                }

                auto childIt = childrenOf.find(id);
                if (childIt != childrenOf.end()) {
                    for (int child : childIt->second) stack.push_back(child);
                }
            }
            return false;
        };

        for (const auto& link : tree.links()) {
            if (!link.is_challenge()) continue;
            int challengeClaimId = link.from_claim_id();
            int targetClaimId = link.to_claim_id();

            auto targetIt = collectionProto.claims_by_id().find(targetClaimId);
            if (targetIt == collectionProto.claims_by_id().end()) continue;
            int targetOwnerId = targetIt->second.creator_id();
            std::string targetOwnerName = userDb.getUsername(targetOwnerId);

            if (isAnswered(challengeClaimId, targetOwnerName)) continue;

            auto challengeIt = collectionProto.claims_by_id().find(challengeClaimId);
            int challengeCreatorId = (challengeIt != collectionProto.claims_by_id().end())
                ? challengeIt->second.creator_id() : -1;

            anyLiveChallenge = true;
            if (viewerUserId > 0 && targetOwnerId == viewerUserId) {
                // Someone is challenging the viewer's claim -> viewer must respond.
                myChallengesToRespond.push_back(challengeClaimId);
            } else if (viewerUserId > 0 && challengeCreatorId == viewerUserId) {
                // The viewer made this challenge -> waiting on the opponent.
                myPendingChallenges.push_back(challengeClaimId);
            }
        }
    }

    // With no live challenges, the root claim's status decides who's ahead:
    //   - viewer IS the root creator and their root claim is FALSE -> they
    //     conceded/lost an exchange and must go on the offensive or lose.
    //   - viewer is NOT the root creator and the creator's root claim is FALSE
    //     -> the creator conceded, so the viewer is winning and just needs the
    //     opponent to stay silent.
    bool mustWinBack = false;  // defender is losing
    bool aboutToWin = false;   // attacker is winning
    if (!anyLiveChallenge && fullDebateInfo.has_full_debate_tree()) {
        int rootId = fullDebateInfo.full_debate_tree().root_claim_id();
        auto rootIt = collectionProto.claims_by_id().find(rootId);
        if (rootIt != collectionProto.claims_by_id().end() && viewerUserId > 0) {
            int rootCreatorId = rootIt->second.creator_id();
            const auto& statuses = rootIt->second.user_statuses();
            if (rootCreatorId == viewerUserId) {
                auto statusIt = statuses.find(viewerUsername);
                if (statusIt != statuses.end() && statusIt->second == debate::ClaimStatus::FALSE_CLAIM) {
                    mustWinBack = true;
                }
            } else {
                std::string rootCreatorName = userDb.getUsername(rootCreatorId);
                auto statusIt = statuses.find(rootCreatorName);
                if (statusIt != statuses.end() && statusIt->second == debate::ClaimStatus::FALSE_CLAIM) {
                    aboutToWin = true;
                }
            }
        }
    }

    // TextComponent applies an inline color that would override a Tailwind
    // text-color class, so the status color is set via the css map (inline)
    // instead. Empty string leaves TextComponent's default color in place.
    std::string statusText;
    std::string statusColorHex;
    if (!anyLiveChallenge) {
        if (mustWinBack) {
            statusText = "No current challenges. If you don't challenge your opponent, you will lose the debate.";
            statusColorHex = "#f97316";  // orange-500
        } else if (aboutToWin) {
            statusText = "No current challenges. If your opponent doesn't challenge you, you will win the debate.";
            statusColorHex = "#22c55e";  // green-500
        } else {
            statusText = "No current challenges.";
            statusColorHex = "";
        }
    } else if (!myChallengesToRespond.empty()) {
        statusText = myChallengesToRespond.size() > 1
            ? "Your opponent has challenged your claims. Respond to each challenge with a counter-challenge, or concede:"
            : "Your opponent has challenged your claim. Respond with a counter-challenge, or concede:";
        statusColorHex = "#f97316";  // orange-500
    } else if (!myPendingChallenges.empty()) {
        statusText = myPendingChallenges.size() > 1
            ? "Waiting for your opponent to respond to your challenges:"
            : "Waiting for your opponent to respond to your challenge:";
        statusColorHex = "";
    } else {
        statusText = "Waiting for your opponent to respond.";
        statusColorHex = "";
    }

    ui::Component statusLine = ComponentGenerator::createText(
        "debateStatusText",
        statusText,
        "text-sm",
        "",
        "",
        ""
    );
    if (!statusColorHex.empty()) {
        (*statusLine.mutable_css())["color"] = statusColorHex;
    }
    ComponentGenerator::addChild(&panel, statusLine);

    // List whichever set of challenges the status line refers to.
    const std::vector<int>& challengesToList =
        !myChallengesToRespond.empty() ? myChallengesToRespond : myPendingChallenges;
    for (int challengeClaimId : challengesToList) {
        std::string sentence;
        auto it = collectionProto.claims_by_id().find(challengeClaimId);
        if (it != collectionProto.claims_by_id().end()) {
            sentence = it->second.sentence();
        }
        ui::Component item = ComponentGenerator::createText(
            "debateStatusChallenge_" + std::to_string(challengeClaimId),
            "\xE2\x80\xA2 " + sentence,
            "text-sm",
            "text-white",
            "",
            ""
        );
        (*item.mutable_css())["overflow-wrap"] = "break-word";
        ComponentGenerator::addChild(&panel, item);
    }

    return panel;
}

// Helper: Build a nested tree of CONTAINER components from the debate tree.
// Each node becomes a CONTAINER with data-tree-node attribute.
// Returns the root component for the subtree rooted at `claimId`.
//
// A claim's PARENT_CHILD descendants nest inline within the same paragraph,
// depth-first, same as before. But when a claim is itself challenged, the
// challenge is NOT nested inline -- it starts a brand new paragraph (its
// sentence becomes that paragraph's first sentence), built with this same
// function so its own descendants and any challenges against IT recurse the
// same way. New paragraphs are appended to `paragraphs` in the order they're
// discovered, i.e. depth-first: a branch's challenges (and challenges on
// those challenges, arbitrarily deep) are fully resolved before moving on to
// the next sibling branch, rather than draining level-by-level.
static ui::Component BuildTreeNode(
    int claimId,
    const rendering_info::FullDebateViewInfo& fullDebateInfo,
    const debate::Collection& collectionProto,
    const std::string& idPrefix,
    std::unordered_set<int>& visited,
    int currentClaimId,
    std::vector<ui::Component>& paragraphs
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

    // Find PARENT_CHILD children in the full_debate_tree, from the
    // convenience adjacency list (challenges are handled separately below).
    std::vector<int> children;
    if (fullDebateInfo.has_full_debate_tree()) {
        const auto& tree = fullDebateInfo.full_debate_tree();
        for (const auto& treeNode : tree.nodes()) {
            if (treeNode.claim_id() == claimId) {
                for (int cid : treeNode.child_claim_ids()) {
                    children.push_back(cid);
                }
                break;
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

    // Recursively build PARENT_CHILD children, depth-first, into this same
    // paragraph.
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

        for (int childId : children) {
            if (visited.count(childId)) continue;
            visited.insert(childId);
            ui::Component childNode = BuildTreeNode(
                childId, fullDebateInfo, collectionProto,
                idPrefix, visited, currentClaimId, paragraphs
            );
            ComponentGenerator::addChild(&childrenContainer, childNode);
        }
        ComponentGenerator::addChild(&node, childrenContainer);
    }

    // Now that this claim's own subtree is fully built, spawn a new paragraph
    // for each claim that challenges it -- depth-first, so each challenge's
    // own subtree (and any further challenges against it) is fully resolved
    // before moving on to the next challenger or sibling branch.
    if (fullDebateInfo.has_full_debate_tree()) {
        const auto& tree = fullDebateInfo.full_debate_tree();
        for (const auto& link : tree.links()) {
            if (!link.is_challenge() || link.to_claim_id() != claimId) continue;
            int challengerId = link.from_claim_id();
            if (visited.count(challengerId)) continue;
            visited.insert(challengerId);
            // Reserve this paragraph's position before recursing, so it lands
            // before any paragraphs its own nested challenges spawn (pure
            // pre-order: outer challenge first, then its challenges, in
            // discovery order) instead of after them.
            size_t insertIndex = paragraphs.size();
            paragraphs.emplace_back();
            ui::Component challengeParagraph = BuildTreeNode(
                challengerId, fullDebateInfo, collectionProto,
                idPrefix, visited, currentClaimId, paragraphs
            );
            paragraphs[insertIndex] = challengeParagraph;
        }
    }

    return node;
}

// The AI summary panel: a button that asks the model to read the whole debate,
// and the result once there is one.
//
// A debate is a graph, and past a dozen claims the graph stops answering "so
// where does this actually stand" at a glance. That is what this is for -- and
// why it sits at the top of the column, above the claim the user happens to
// have selected.
// Renders one summary's contents -- pending, error, or the text itself -- into
// `panel`. Shared by the debate summary and the per-claim summary: they differ
// in what they summarize and where they sit, not in how a summary looks.
//
// `idPrefix` keeps component ids unique between the two, and `pendingMarkerId`
// is what the client polls on while a run is going.
static void AddSummaryBody(ui::Component* panel,
                           const user_engagement::DebatingInfo::DebateSummary& summary,
                           const std::string& idPrefix,
                           const std::string& pendingMarkerId,
                           const std::string& emptyHint,
                           const std::string& pendingHint) {
    if (summary.pending()) {
        ui::Component progress = ComponentGenerator::createText(
            pendingMarkerId,
            pendingHint + " " + std::to_string(summary.elapsed_seconds()) + "s elapsed",
            "text-sm", "text-indigo-300", "", ""
        );
        ComponentGenerator::addChild(panel, progress);
        ui::Component hint = ComponentGenerator::createText(
            idPrefix + "PendingHint",
            "The local model usually takes 30-90 seconds. You can keep working; "
            "it will appear here when it is done.",
            "text-xs", "text-gray-500", "", ""
        );
        ComponentGenerator::addChild(panel, hint);
        return;
    }

    if (!summary.error().empty()) {
        ui::Component errorText = ComponentGenerator::createText(
            idPrefix + "Error", summary.error(), "text-sm", "text-red-400", "", ""
        );
        (*errorText.mutable_css())["overflow-wrap"] = "anywhere";
        ComponentGenerator::addChild(panel, errorText);
        return;
    }

    if (summary.text().empty()) {
        ui::Component hint = ComponentGenerator::createText(
            idPrefix + "Hint", emptyHint, "text-sm", "text-gray-400", "", ""
        );
        ComponentGenerator::addChild(panel, hint);
        return;
    }

    // Now that summaries persist, one can outlive the debate it describes. Say
    // so rather than letting it quietly misrepresent the current state.
    if (summary.moves_behind() > 0) {
        const int behind = summary.moves_behind();
        ui::Component stale = ComponentGenerator::createText(
            idPrefix + "Stale",
            "This summary is " + std::to_string(behind) + (behind == 1 ? " move" : " moves") +
                " out of date. Press Re-summarize for the current state.",
            "text-xs", "text-amber-400", "", ""
        );
        ComponentGenerator::addChild(panel, stale);
    }

    // One component per line. The renderer collapses whitespace like HTML, so a
    // single text node would run the model's sections together into one
    // paragraph and lose the structure the prompt asked for.
    ui::Component body = ComponentGenerator::createContainer(
        idPrefix + "Body", "w-full", "", "", "", "", "", ""
    );
    (*body.mutable_css())["display"] = "flex";
    (*body.mutable_css())["flex-direction"] = "column";
    (*body.mutable_css())["gap"] = "0.15rem";

    std::istringstream lines(summary.text());
    std::string line;
    int lineIndex = 0;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const std::string id = idPrefix + "Line_" + std::to_string(lineIndex++);
        if (line.empty()) {
            ui::Component spacer = ComponentGenerator::createContainer(id, "", "", "", "", "", "", "");
            (*spacer.mutable_css())["height"] = "0.4rem";
            ComponentGenerator::addChild(&body, spacer);
            continue;
        }

        // The prompt asks for ALL-CAPS section headings; styling them is what
        // makes the summary skimmable rather than a wall of text.
        const bool isHeading = line.find_first_of("abcdefghijklmnopqrstuvwxyz") == std::string::npos;
        ui::Component textLine = ComponentGenerator::createText(
            id, line, isHeading ? "text-xs" : "text-sm",
            isHeading ? "text-gray-400" : "text-gray-100",
            isHeading ? "font-semibold uppercase tracking-wide" : "", ""
        );
        (*textLine.mutable_css())["overflow-wrap"] = "anywhere";
        if (isHeading && lineIndex > 1) {
            (*textLine.mutable_css())["margin-top"] = "0.35rem";
        }
        ComponentGenerator::addChild(&body, textLine);
    }

    ComponentGenerator::addChild(panel, body);
}

static ui::Component BuildDebateSummaryPanel(const user::User& viewerUser) {
    const user_engagement::DebatingInfo::DebateSummary& summary =
        viewerUser.engagement().debating_info().debate_summary();
    const bool pending = summary.pending();
    const bool hasText = !pending && !summary.text().empty();
    const bool hasError = !pending && !summary.error().empty();

    ui::Component panel = ComponentGenerator::createContainer(
        "debateSummaryPanel", "w-full", "bg-gray-800", "p-4", "",
        "border border-gray-700", "rounded-lg", ""
    );
    (*panel.mutable_css())["display"] = "flex";
    (*panel.mutable_css())["flex-direction"] = "column";
    (*panel.mutable_css())["gap"] = "0.5rem";

    ui::Component headerRow = ComponentGenerator::createContainer(
        "debateSummaryHeaderRow", "flex gap-2", "", "", "", "", "", ""
    );
    (*headerRow.mutable_css())["display"] = "flex";
    (*headerRow.mutable_css())["align-items"] = "center";
    (*headerRow.mutable_css())["justify-content"] = "space-between";
    (*headerRow.mutable_css())["gap"] = "0.5rem";

    ui::Component heading = ComponentGenerator::createText(
        "debateSummaryHeading", "AI Summary of Debate:", "text-xs", "text-gray-500",
        "font-semibold uppercase tracking-wide", ""
    );
    ComponentGenerator::addChild(&headerRow, heading);

    ui::Component buttonGroup = ComponentGenerator::createContainer(
        "debateSummaryButtons", "flex gap-2", "", "", "", "", "", ""
    );
    (*buttonGroup.mutable_css())["display"] = "flex";
    (*buttonGroup.mutable_css())["gap"] = "0.5rem";

    // The label says what a second press does, because the debate moves and a
    // summary taken five claims ago is not the summary of this debate.
    // While a run is going the button says so and stops inviting a second
    // press -- pressing again cannot start anything (AiJobs refuses a
    // concurrent run) and would only look broken.
    // Sits with the summary controls because both are "ask the AI about this
    // debate" -- one describes it, the other tries to break it.
    const user_engagement::AiJobStatus& aiJob = viewerUser.engagement().ai_job();
    ui::Component challengeButton = ComponentGenerator::createButton(
        "aiChallengeButton", aiJob.pending() ? "Working..." : "AI Challenge", "",
        aiJob.pending() ? "bg-gray-600" : "bg-orange-700",
        aiJob.pending() ? "hover:bg-gray-600" : "hover:bg-orange-800", "text-white",
        "px-3 py-1.5", "rounded", "text-sm"
    );
    ComponentGenerator::addChild(&buttonGroup, challengeButton);

    ui::Component summarizeButton = ComponentGenerator::createButton(
        "summarizeDebateButton",
        pending ? "Summarizing..." : ((hasText || hasError) ? "Re-summarize" : "Summarize Debate"),
        "", pending ? "bg-gray-600" : "bg-indigo-600",
        pending ? "hover:bg-gray-600" : "hover:bg-indigo-700", "text-white",
        "px-3 py-1.5", "rounded", "text-sm"
    );
    ComponentGenerator::addChild(&buttonGroup, summarizeButton);

    if (!pending && (hasText || hasError)) {
        ui::Component dismissButton = ComponentGenerator::createButton(
            "dismissDebateSummaryButton", "Dismiss", "", "bg-gray-700",
            "hover:bg-gray-600", "text-gray-200", "px-3 py-1.5", "rounded", "text-sm"
        );
        ComponentGenerator::addChild(&buttonGroup, dismissButton);
    }

    ComponentGenerator::addChild(&headerRow, buttonGroup);
    ComponentGenerator::addChild(&panel, headerRow);

    // A marker that lives as long as ANY summary run does, so the client keeps
    // polling until every result has been collected -- including a claim
    // summary still generating while this panel already shows its answer.
    // A run can take minutes on a local model. Without this the only way out of
    // a mistaken click is to wait it out.
    if (aiJob.pending() || summary.pending() ||
        viewerUser.engagement().debating_info().summary_running()) {
        ComponentGenerator::addChild(&panel, ComponentGenerator::createButton(
            "cancelAiWorkButton", "Cancel", "", "bg-gray-700", "hover:bg-gray-600",
            "text-gray-200", "px-3 py-1.5", "rounded", "text-sm"));
    }

    if (aiJob.pending()) {
        ui::Component working = ComponentGenerator::createText(
            "aiChallengeSummaryPending",
            "The AI is reading the debate and looking for weaknesses... " +
                std::to_string(aiJob.elapsed_seconds()) + "s elapsed",
            "text-sm", "text-orange-300", "", ""
        );
        ComponentGenerator::addChild(&panel, working);
    }
    // A notice describes work in ONE debate. Without this check, "Generated a
    // debate from 20 steps" followed the user into an unrelated debate and made
    // a hand-built one look AI-written.
    const int viewingDebateId = viewerUser.engagement().debating_info().debate_id();
    const bool noticeIsAboutThisDebate =
        aiJob.about_debate_id() == 0 || aiJob.about_debate_id() == viewingDebateId;
    if (!aiJob.note().empty() && noticeIsAboutThisDebate) {
        ComponentGenerator::addChild(&panel, ComponentGenerator::createText(
            "aiChallengeNote", aiJob.note(), "text-sm", "text-green-400", "", ""));
        ComponentGenerator::addChild(&panel, ComponentGenerator::createButton(
            "dismissAiNoticeButton", "Dismiss", "", "bg-gray-700", "hover:bg-gray-600",
            "text-gray-200", "px-3 py-1.5", "rounded", "text-sm"));
    }
    if (!aiJob.error().empty()) {
        ui::Component err = ComponentGenerator::createText(
            "aiChallengeError", aiJob.error(), "text-sm", "text-red-400", "", "");
        (*err.mutable_css())["overflow-wrap"] = "anywhere";
        ComponentGenerator::addChild(&panel, err);
    }

    if (viewerUser.engagement().debating_info().summary_running() && !summary.pending()) {
        ui::Component working = ComponentGenerator::createText(
            "backgroundSummaryPending", "An AI summary is still generating...",
            "text-xs", "text-indigo-300", "", ""
        );
        ComponentGenerator::addChild(&panel, working);
    }

    AddSummaryBody(&panel, summary, "debateSummary", "debateSummaryPending",
                   "No summary yet. Press Summarize Debate to have the AI read the whole debate.",
                   "Reading the debate and writing a summary...");

    return panel;
}

// The per-claim summary, shown inside the selected-claim panel. Separate from
// the whole-debate one because it answers a different question -- what is THIS
// claim doing here -- and a reader may want both at once.
static ui::Component BuildClaimSummarySection(const user::User& viewerUser, int selectedClaimId) {
    const user_engagement::DebatingInfo& info = viewerUser.engagement().debating_info();
    const user_engagement::DebatingInfo::DebateSummary& summary = info.claim_summary();

    // A summary belongs to the claim it was asked about. Showing it beside a
    // different claim would attach an explanation to the wrong sentence, which
    // is worse than showing nothing.
    const bool aboutThisClaim = info.claim_summary_claim_id() == selectedClaimId;
    const bool pending = aboutThisClaim && summary.pending();
    const bool hasContent =
        aboutThisClaim && !pending && (!summary.text().empty() || !summary.error().empty());

    ui::Component section = ComponentGenerator::createContainer(
        "claimSummarySection", "w-full", "", "", "", "", "", ""
    );
    (*section.mutable_css())["display"] = "flex";
    (*section.mutable_css())["flex-direction"] = "column";
    (*section.mutable_css())["gap"] = "0.35rem";
    (*section.mutable_css())["margin-top"] = "0.5rem";
    (*section.mutable_css())["padding-top"] = "0.5rem";
    (*section.mutable_css())["border-top"] = "1px solid #374151";

    ui::Component headerRow = ComponentGenerator::createContainer(
        "claimSummaryHeaderRow", "flex gap-2", "", "", "", "", "", ""
    );
    (*headerRow.mutable_css())["display"] = "flex";
    (*headerRow.mutable_css())["align-items"] = "center";
    (*headerRow.mutable_css())["justify-content"] = "space-between";
    (*headerRow.mutable_css())["gap"] = "0.5rem";

    ui::Component heading = ComponentGenerator::createText(
        "claimSummaryHeading", "AI Summary of This Claim:", "text-xs", "text-gray-500",
        "font-semibold uppercase tracking-wide", ""
    );
    ComponentGenerator::addChild(&headerRow, heading);

    ui::Component buttons = ComponentGenerator::createContainer(
        "claimSummaryButtons", "flex gap-2", "", "", "", "", "", ""
    );
    (*buttons.mutable_css())["display"] = "flex";
    (*buttons.mutable_css())["gap"] = "0.5rem";

    // The id carries the claim id, so the request is about the claim that was
    // on screen when it was pressed rather than whatever is selected later.
    ui::Component summarizeButton = ComponentGenerator::createButton(
        "summarizeClaimButton_" + std::to_string(selectedClaimId),
        pending ? "Summarizing..." : (hasContent ? "Re-summarize" : "Summarize This Claim"),
        "", pending ? "bg-gray-600" : "bg-indigo-600",
        pending ? "hover:bg-gray-600" : "hover:bg-indigo-700", "text-white",
        "px-3 py-1.5", "rounded", "text-sm"
    );
    ComponentGenerator::addChild(&buttons, summarizeButton);

    if (hasContent) {
        ui::Component dismissButton = ComponentGenerator::createButton(
            "dismissClaimSummaryButton", "Dismiss", "", "bg-gray-700", "hover:bg-gray-600",
            "text-gray-200", "px-3 py-1.5", "rounded", "text-sm"
        );
        ComponentGenerator::addChild(&buttons, dismissButton);
    }

    ComponentGenerator::addChild(&headerRow, buttons);
    ComponentGenerator::addChild(&section, headerRow);

    if (aboutThisClaim) {
        AddSummaryBody(&section, summary, "claimSummary", "claimSummaryPending",
                       "", "Reading this claim and its surroundings...");
    }

    return section;
}


// Builds the walkthrough guide box: where you are, what just happened, and the
// button that makes the next thing happen.
//
// The words come from the .pbtxt on every render rather than from anything
// stored on the user, so editing a script changes what a walkthrough already
// in progress says. Only the reader's POSITION is saved.
//
// Returns false into `show` when the viewer is not in a walkthrough, or is in
// one that belongs to a different debate -- opening another debate mid-tutorial
// should not drag the guide box along, narrating steps about claims that are
// not on screen.
// Two small helpers so the guide box can set real CSS rather than fight the
// Tailwind class strings. The renderer turns a component's css map into an
// inline style, and inline style is what reliably wins here -- TextComponent
// substitutes its own colour and font size for anything it is not given
// explicitly.
static void tutorialCss(ui::Component* component,
                        const std::vector<std::pair<std::string, std::string>>& rules) {
    for (const auto& rule : rules) {
        (*component->mutable_css())[rule.first] = rule.second;
    }
}

static ui::Component tutorialText(const std::string& id, const std::string& text,
                                  const std::vector<std::pair<std::string, std::string>>& rules) {
    ui::Component component = ComponentGenerator::createText(id, text, "", "", "", "");
    tutorialCss(&component, rules);
    return component;
}

// Builds the walkthrough guide box: where you are, what just happened, and the
// button that makes the next thing happen.
//
// It is styled loudly on purpose. This box is the only thing on the page a
// first-time reader is meant to follow, and it competes with a graph, a claim
// panel and a summary panel for the same eye -- a quiet bordered rectangle
// lost that competition.
//
// The words come from the .pbtxt on every render rather than from anything
// stored on the user, so editing a script changes what a walkthrough already
// in progress says. Only the reader's POSITION is saved.
//
// Returns false into `show` when the viewer is not in a walkthrough, or is in
// one that belongs to a different debate -- opening another debate mid-tutorial
// should not drag the guide box along, narrating steps about claims that are
// not on screen.
static ui::Component BuildTutorialPanel(const user::User& viewerUser, bool& show) {
    ui::Component panel;
    show = false;

    const user_engagement::TutorialStatus& state = viewerUser.engagement().tutorial();
    if (!state.active()) {
        return panel;
    }
    // Which debate the reader is actually looking at. FullDebateViewInfo does
    // not carry the debate id, and the viewer's engagement is the authority on
    // where they are anyway.
    const int thisDebateId = viewerUser.engagement().debating_info().debate_id();
    if (state.debate_id() > 0 && thisDebateId > 0 && state.debate_id() != thisDebateId) {
        return panel;
    }
    show = true;

    // The sticky rules ride on a class, not the css map, so they can vary by
    // screen size -- see .tutorial-guide-sticky in index.css.
    panel = ComponentGenerator::createContainer(
        "tutorialPanel", "w-full", "", "", "", "", "", "tutorial-guide-sticky");
    tutorialCss(&panel, {
        {"display", "flex"},
        {"flex-direction", "column"},
        {"gap", "0.75rem"},
        {"background", "linear-gradient(135deg, #0c1f3d 0%, #14294f 55%, #0b1a33 100%)"},
        // Each side separately, NOT `border` plus a `border-left` override.
        // These rules travel in a protobuf map, whose iteration order is
        // unspecified, and the renderer turns that map straight into a style
        // object -- so the shorthand could be applied after the longhand and
        // silently flatten the accent bar back to a 1px blue line. It did.
        {"border-top", "1px solid #2563eb"},
        {"border-right", "1px solid #2563eb"},
        {"border-bottom", "1px solid #2563eb"},
        // The accent bar down the side is what makes this read as a panel to
        // follow rather than one more bordered box among the four on screen.
        {"border-left", "5px solid #38bdf8"},
        {"border-radius", "0.75rem"},
        {"padding", "1rem 1.25rem"},
        {"box-shadow", "0 0 0 1px rgba(56,189,248,0.15), 0 10px 28px rgba(2,132,199,0.28)"},
        // Scrollable inside itself once .tutorial-guide-sticky caps its height.
        // The shorthand, never overflow-y: these rules ride in a protobuf map
        // with no defined order, and ContainerComponent writes its own
        // `overflow` afterwards from this exact key. A longhand here would be
        // flattened by that shorthand -- the same trap the border hit.
        {"overflow", "auto"},
    });

    bool ok = false;
    std::string loadError;
    const tutorial::Tutorial script = tutorials::Load(state.script(), ok, loadError);

    if (!ok) {
        ComponentGenerator::addChild(&panel, tutorialText("tutorialLoadError", loadError,
            {{"color", "#fca5a5"}, {"font-size", "0.9rem"}}));
        ComponentGenerator::addChild(&panel, ComponentGenerator::createButton(
            "tutorialExitButton_load", "Leave tutorial", "", "", "", "", "", "", "text-sm"));
        return panel;
    }

    const int index = state.step_index();
    const int total = script.steps_size();

    // ---- Header: step pill, progress bar, exit ----------------------------
    ui::Component headerRow = ComponentGenerator::createContainer(
        "tutorialHeaderRow", "w-full", "", "", "", "", "", "");
    tutorialCss(&headerRow, {
        {"display", "flex"}, {"flex-direction", "row"},
        {"align-items", "center"}, {"gap", "0.75rem"},
    });

    ComponentGenerator::addChild(&headerRow, tutorialText(
        "tutorialProgress",
        "STEP " + std::to_string(index + 1) + " OF " + std::to_string(total),
        {{"background", "linear-gradient(90deg, #0ea5e9, #38bdf8)"},
         {"color", "#04121f"},
         {"font-size", "0.7rem"},
         {"font-weight", "800"},
         {"letter-spacing", "0.08em"},
         {"padding", "0.2rem 0.65rem"},
         {"border-radius", "9999px"},
         {"white-space", "nowrap"},
         {"margin", "0"},
         {"flex", "0 0 auto"}}));

    // A bar rather than only the "3 of 8" text: how much is left is the thing
    // a reader wants to know before committing to a walkthrough, and a number
    // makes them do the arithmetic.
    ui::Component progressTrack = ComponentGenerator::createContainer(
        "tutorialProgressTrack", "", "", "", "", "", "", "");
    tutorialCss(&progressTrack, {
        {"flex", "1 1 auto"}, {"height", "6px"},
        {"background", "rgba(148, 197, 253, 0.18)"},
        {"border-radius", "9999px"}, {"overflow", "hidden"},
        {"min-width", "60px"},
    });
    ui::Component progressFill = ComponentGenerator::createContainer(
        "tutorialProgressFill", "", "", "", "", "", "", "");
    const int percent = total > 0 ? ((index + 1) * 100) / total : 0;
    tutorialCss(&progressFill, {
        {"width", std::to_string(percent) + "%"},
        {"height", "6px"},
        {"background", "linear-gradient(90deg, #38bdf8, #22d3ee)"},
        {"border-radius", "9999px"},
    });
    ComponentGenerator::addChild(&progressTrack, progressFill);
    ComponentGenerator::addChild(&headerRow, progressTrack);

    ui::Component exitButton = ComponentGenerator::createButton(
        "tutorialExitButton", "Exit", "", "", "", "", "", "", "text-xs");
    tutorialCss(&exitButton, {
        {"background", "transparent"},
        {"color", "#93a9c9"},
        {"border", "1px solid rgba(148, 197, 253, 0.3)"},
        {"flex", "0 0 auto"},
    });
    ComponentGenerator::addChild(&headerRow, exitButton);
    ComponentGenerator::addChild(&panel, headerRow);

    // ---- The step itself ---------------------------------------------------
    if (index >= 0 && index < total) {
        const tutorial::TutorialStep& step = script.steps(index);
        if (!step.title().empty()) {
            ComponentGenerator::addChild(&panel, tutorialText("tutorialStepTitle", step.title(),
                {{"color", "#f1f7ff"},
                 {"font-size", "1.4rem"},
                 {"font-weight", "800"},
                 {"letter-spacing", "-0.01em"},
                 {"line-height", "1.25"},
                 {"margin", "0"}}));
        }
        int paragraph = 0;
        for (const std::string& body : step.body()) {
            ComponentGenerator::addChild(&panel, tutorialText(
                "tutorialBody_" + std::to_string(paragraph++), body,
                {{"color", "#cbd9ee"}, {"font-size", "0.95rem"},
                 {"line-height", "1.65"}, {"margin", "0"}}));
        }
        if (!step.look_at().empty()) {
            // Amber, and boxed. This is the one line that asks the reader to
            // move their eyes off the panel, so it should not look like more
            // prose.
            ui::Component lookAt = ComponentGenerator::createContainer(
                "tutorialLookAtBox", "", "", "", "", "", "", "");
            tutorialCss(&lookAt, {
                {"background", "rgba(250, 204, 21, 0.10)"},
                {"border-left", "3px solid #facc15"},
                {"border-radius", "0.375rem"},
                {"padding", "0.5rem 0.75rem"},
            });
            ComponentGenerator::addChild(&lookAt, tutorialText(
                "tutorialLookAt", "Look at " + step.look_at(),
                {{"color", "#fde68a"}, {"font-size", "0.9rem"},
                 {"font-weight", "600"}, {"margin", "0"}}));
            ComponentGenerator::addChild(&panel, lookAt);
        }
    }

    // An error replaces the Next button rather than sitting beside it: the
    // walkthrough did not advance, and offering Next again would just fail
    // again in the same place.
    if (!state.error().empty()) {
        ComponentGenerator::addChild(&panel, tutorialText("tutorialError", state.error(),
            {{"color", "#fca5a5"}, {"font-size", "0.9rem"}, {"margin", "0"}}));
        ComponentGenerator::addChild(&panel, ComponentGenerator::createButton(
            "tutorialExitButton_error", "Leave tutorial", "", "", "", "", "", "", "text-sm"));
        return panel;
    }

    if (state.finished()) {
        ComponentGenerator::addChild(&panel, tutorialText("tutorialDone",
            "That is the whole walkthrough. The debate it built is yours -- keep clicking "
            "around it, or leave the tutorial and it stays in your list.",
            {{"color", "#86efac"}, {"font-size", "0.95rem"}, {"margin", "0"}}));
        ui::Component finish = ComponentGenerator::createButton(
            "tutorialExitButton_finish", "Finish", "", "", "", "", "", "", "text-lg");
        tutorialCss(&finish, {
            {"background", "linear-gradient(90deg, #16a34a, #22c55e)"},
            {"color", "#ffffff"},
            {"border", "none"},
            {"align-self", "flex-start"},
            {"box-shadow", "0 4px 14px rgba(22, 163, 74, 0.45)"},
        });
        ComponentGenerator::addChild(&panel, finish);
    } else {
        // The one control the reader is meant to press, so it is the brightest
        // thing in the box and says where it goes.
        ui::Component next = ComponentGenerator::createButton(
            "tutorialNextButton", "Next step  \xE2\x86\x92", "", "", "", "", "", "", "text-lg");
        tutorialCss(&next, {
            {"background", "linear-gradient(90deg, #2563eb, #0ea5e9)"},
            {"color", "#ffffff"},
            {"border", "none"},
            {"align-self", "flex-start"},
            {"font-weight", "700"},
            {"box-shadow", "0 4px 16px rgba(37, 99, 235, 0.5)"},
        });
        ComponentGenerator::addChild(&panel, next);
    }

    return panel;
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
	// ContainerComponent gives every container overflow:hidden unless the css
	// map overrides it, and an ancestor with any non-visible overflow becomes
	// the scroll container a sticky descendant sticks to. This one never
	// scrolls, so the tutorial panel would have stuck to a box that does not
	// move -- which looks exactly like sticky not working.
	(*container.mutable_css())["overflow"] = "visible";

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

	// Above the argument rather than beside it: the guide box is what the
	// reader is following, and in the right-hand column it would compete with
	// the graph for the same eye and lose.
	bool showTutorial = false;
	ui::Component tutorialPanel =
		BuildTutorialPanel(viewerUser, showTutorial);
	if (showTutorial) {
		ComponentGenerator::addChild(&container, tutorialPanel);
	}

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
	(*leftColumn.mutable_css())["flex"] = "1 1 40%";
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
	// The graph is the point of the tool, so it gets the larger share. It was
	// 45% with a 400px floor, which in a narrow window left the argument itself
	// rendering into ~314px -- narrow enough that a 22-claim debate could only
	// be fitted by shrinking past the point where any label is drawn.
	(*rightColumn.mutable_css())["flex"] = "1 1 60%";
	(*rightColumn.mutable_css())["min-width"] = "420px";


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

	ui::Component debateStatusPanel = BuildDebateStatusPanel(
		fullDebateInfo, collectionProto, userDb, fullDebateInfo.viewer_user_id(), fullDebateInfo.viewer_username()
	);
	ComponentGenerator::addChild(&leftColumn, debateStatusPanel);

	ui::Component debateSummaryPanel = BuildDebateSummaryPanel(viewerUser);
	ComponentGenerator::addChild(&leftColumn, debateSummaryPanel);

	// A selected ARGUMENT takes the claim panel's place. The selection is
	// only honoured if the collection still has that relation -- a stale id
	// after a deletion falls back to the claim rather than an empty card.
	const int selectedLinkId = viewerUser.engagement().debating_info().selected_link_id();
	const bool showRelation = selectedLinkId > 0 &&
		collectionProto.links_by_id().count(selectedLinkId) > 0;
	ui::Component selectedClaimPanel = showRelation
		? BuildSelectedRelationPanel(
			selectedLinkId, collectionProto, userDb, fullDebateInfo.viewer_user_id(), fullDebateInfo.viewer_username())
		: BuildSelectedClaimPanel(
			currentClaimId, fullDebateInfo, collectionProto, userDb, fullDebateInfo.viewer_user_id(), fullDebateInfo.viewer_username());
	// Attached at the call site rather than inside BuildSelectedClaimPanel,
	// which is defined further up the file and would otherwise need a forward
	// declaration and a wider signature just to reach the viewer.
	if (!showRelation && currentClaimId > 0) {
		ComponentGenerator::addChild(&selectedClaimPanel,
			BuildClaimSummarySection(viewerUser, currentClaimId));
	}
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
		std::vector<ui::Component> paragraphs;
		ui::Component treeRoot = BuildTreeNode(
			rootClaimId, fullDebateInfo, collectionProto,
			"debateTree", visited, currentClaimId, paragraphs
		);
		paragraphs.insert(paragraphs.begin(), treeRoot);

		// One paragraph per entry: the main argument first, then one new
		// paragraph per challenge encountered (depth-first), each starting
		// with that challenge's own sentence.
		for (size_t i = 0; i < paragraphs.size(); ++i) {
			ui::Component& paragraph = paragraphs[i];
			ComponentGenerator::addAttribute(&paragraph, "data-tree-root", "true");
			if (i > 0) {
				(*paragraph.mutable_css())["margin-top"] = "0.75rem";
				(*paragraph.mutable_css())["padding-top"] = "0.75rem";
				(*paragraph.mutable_css())["border-top"] = "1px dashed #4b5563";
			}
			ComponentGenerator::addChild(&treeWrapper, paragraph);
		}
		ComponentGenerator::addChild(&leftColumn, treeWrapper);
	}

	const int mapFocusClaimId = currentClaimId > 0
		? currentClaimId
		: (fullDebateInfo.steps_size() > 0 ? fullDebateInfo.steps(0).claim_id() : 0);
	const float mapScale = 0.82f;
	// A selected argument is highlighted in the graph in place of the claim.
	const int graphSelectedLink = viewerUser.engagement().debating_info().selected_link_id();
	ui::Component mapSection = FullDebatePageGenerator::GenerateMapSection(
		fullDebateInfo, mapFocusClaimId, mapScale, &userDb,
		graphSelectedLink > 0 ? "r" + std::to_string(graphSelectedLink) : std::string());
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
