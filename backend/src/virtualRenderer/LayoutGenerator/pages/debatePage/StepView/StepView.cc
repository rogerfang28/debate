#include <algorithm>
#include <cctype>
#include <set>
#include "../../../../../utils/DemoMode.h"
#include "../../../../../utils/AnonymousUser.h"
#include "../../../../../utils/AiAvailability.h"
#include "../../../../../utils/ClaimVoice.h"
#include "../../../../../utils/DebateChat.h"
#include "../../../../../utils/Ordered.h"
#include "StepView.h"
#include "../../../ComponentGenerator.h"
#include "../FullDebateView/FullDebatePageGenerator.h"
#include "../FullDebateView/FullDebatePageInfoParser.h"
#include "../../../../utils/UserNameResolver.h"
#include "../../../../../utils/TutorialLibrary.h"
#include "../../../../../utils/UsernamePolicy.h"
#include <map>
#include <set>
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
    if (viewerUserId == ClaimVoiceId(claim)) {
        return debate::ClaimStatus::TRUE_CLAIM;
    }
    return debate::ClaimStatus::UNDETERMINED;
}

// Why a claim has the status it has for the viewer, in one sentence, from
// what they hold and the claims around it; and, if two things they hold
// contradict each other here, a warning saying which. Mirrors the status
// engine's rules (StatusEngine.cc A-D) in words: a challenge you agree with
// defeats the claim, even one you also agreed with; a reason that is false
// for you sinks it; rejecting a challenge, or accepting every reason, makes
// it true.
struct StatusExplanation {
    std::string why;
    std::string conflict;
};

static std::string Quoted(const std::string& text) {
    std::string t = text;
    while (!t.empty() && (t.back() == '.' || t.back() == ' ')) t.pop_back();
    if (t.size() > 110) {
        size_t cut = t.rfind(' ', 105);
        t = t.substr(0, cut == std::string::npos ? 105 : cut) + "\u2026";
    }
    return "\u201c" + t + "\u201d";
}

static StatusExplanation ExplainStatusForViewer(
    int claimId,
    const rendering_info::FullDebateViewInfo& fullDebateInfo,
    const debate::Collection& collectionProto,
    VRUserDatabase& userDb,
    int viewerUserId,
    const std::string& viewerUsername
) {
    StatusExplanation out;
    const auto& claims = collectionProto.claims_by_id();
    auto self = claims.find(claimId);
    if (self == claims.end()) return out;

    std::set<int> held(fullDebateInfo.viewer_held_claim_ids().begin(),
                       fullDebateInfo.viewer_held_claim_ids().end());
    auto holds = [&](int id) {
        if (held.count(id)) return true;
        auto it = claims.find(id);
        return it != claims.end() && ClaimVoiceId(it->second) == viewerUserId;
    };
    auto statusOf = [&](int id) {
        auto it = claims.find(id);
        return it == claims.end() ? debate::ClaimStatus::UNDETERMINED
                                  : StepViewMapClaimStatusForUser(it->second, viewerUserId, viewerUsername);
    };
    auto textOf = [&](int id) {
        auto it = claims.find(id);
        return it == claims.end() ? std::string() : it->second.sentence();
    };
    auto whoOf = [&](int id) {
        auto it = claims.find(id);
        if (it == claims.end()) return std::string("someone");
        const std::string name = username_policy::DisplayName(userDb.getUsername(ClaimVoiceId(it->second)));
        return name.empty() ? std::string("someone") : name;
    };

    std::vector<int> challengers, reasons, targets;
    for (const auto& link : fullDebateInfo.full_debate_tree().links()) {
        if (link.is_challenge()) {
            if (link.to_claim_id() == claimId && link.to_relation_id() == 0) challengers.push_back(link.from_claim_id());
            if (link.from_claim_id() == claimId && link.to_relation_id() == 0) targets.push_back(link.to_claim_id());
        } else if (link.to_claim_id() == claimId) {
            reasons.push_back(link.from_claim_id());
        }
    }

    const debate::ClaimStatus status = statusOf(claimId);
    const bool youHold = holds(claimId);
    if (status == debate::ClaimStatus::FALSE_CLAIM) {
        for (int c : challengers) {
            if (statusOf(c) == debate::ClaimStatus::TRUE_CLAIM) {
                out.why = "You agree with " + whoOf(c) + "'s challenge to it: " + Quoted(textOf(c)) + ".";
                if (youHold) {
                    out.conflict = "You also agreed with this claim, but agreeing with a challenge to it outweighs that. "
                                   "Withdraw one of the two to settle it.";
                }
                break;
            }
        }
        if (out.why.empty()) {
            for (int r : reasons) {
                if (statusOf(r) == debate::ClaimStatus::FALSE_CLAIM) {
                    out.why = "A reason it rests on is false for you: " + Quoted(textOf(r)) + ".";
                    break;
                }
            }
        }
        if (out.why.empty()) out.why = "Something you hold makes it false for you.";
    } else if (status == debate::ClaimStatus::TRUE_CLAIM) {
        if (youHold) {
            out.why = ClaimVoiceId(self->second) == viewerUserId ? "It's your claim." : "You agreed with it.";
        } else {
            for (int c : challengers) {
                if (statusOf(c) == debate::ClaimStatus::FALSE_CLAIM) {
                    out.why = "You reject " + whoOf(c) + "'s challenge to it: " + Quoted(textOf(c)) + ".";
                    break;
                }
            }
            if (out.why.empty() && !reasons.empty()) {
                bool allTrue = true;
                for (int r : reasons) allTrue = allTrue && statusOf(r) == debate::ClaimStatus::TRUE_CLAIM;
                if (allTrue) out.why = "You accept every reason given for it.";
            }
            if (out.why.empty()) out.why = "Something you hold makes it true for you.";
        }
        // What holding this does to the claims it challenges.
        for (int t : targets) {
            if (holds(t) && statusOf(t) == debate::ClaimStatus::FALSE_CLAIM) {
                out.conflict = "Because you agree with this, " + Quoted(textOf(t)) +
                               ", which you also agreed with, is false for you. Withdraw one of the two to settle it.";
                break;
            }
        }
    } else {
        out.why = "You haven't taken a side on it, and nothing you hold decides it.";
    }
    return out;
}

// One inspector row whose value is people: each name a link to their
// profile (openProfile_<where>_<user id>), comma-separated. A visitor or a
// closed account has no profile, so theirs is plain text.
static ui::Component BuildPeopleField(const std::string& idPrefix, const std::string& idSuffix,
                                      const std::string& label,
                                      const std::vector<std::pair<std::string, int>>& people) {
    ui::Component row = ComponentGenerator::createContainer(
        idPrefix + idSuffix + "Row", "", "", "", "", "", "", "");
    (*row.mutable_css())["display"] = "flex";
    (*row.mutable_css())["flex-direction"] = "column";
    (*row.mutable_css())["gap"] = "0.15rem";
    ComponentGenerator::addChild(&row, ComponentGenerator::createText(
        idPrefix + idSuffix + "Label", label, "text-xs", "text-gray-500", "font-semibold uppercase tracking-wide", ""));

    ui::Component names = ComponentGenerator::createContainer(
        idPrefix + idSuffix + "Value", "", "", "", "", "", "", "");
    (*names.mutable_css())["display"] = "flex";
    (*names.mutable_css())["flex-direction"] = "row";
    (*names.mutable_css())["flex-wrap"] = "wrap";
    (*names.mutable_css())["align-items"] = "baseline";
    (*names.mutable_css())["gap"] = "0";
    for (size_t i = 0; i < people.size(); ++i) {
        const std::string& name = people[i].first;
        const int userId = people[i].second;
        const std::string key = idPrefix + idSuffix + std::to_string(i);
        const bool linked = userId > 0 && !username_policy::IsDeletedName(name) &&
                            !anonymous_user::IsAnonymousUsername(name);
        if (linked) {
            ui::Component link = ComponentGenerator::createButton(
                "openProfile_" + key + "_" + std::to_string(userId), name, "", "", "", "", "", "", "text-sm");
            (*link.mutable_css())["background"] = "transparent";
            (*link.mutable_css())["border"] = "none";
            (*link.mutable_css())["padding"] = "0";
            (*link.mutable_css())["color"] = "var(--accent)";
            (*link.mutable_css())["cursor"] = "pointer";
            (*link.mutable_css())["font-size"] = "14px";
            (*link.mutable_css())["font-weight"] = "400";
            (*link.mutable_css())["text-align"] = "left";
            (*link.mutable_css())["overflow-wrap"] = "anywhere";
            ComponentGenerator::addAttribute(&link, "title", "View profile");
            ComponentGenerator::addChild(&names, link);
        } else {
            ui::Component plain = ComponentGenerator::createText(
                key + "Name", username_policy::DisplayName(name), "text-sm", "text-white", "", "");
            (*plain.mutable_css())["margin"] = "0";
            ComponentGenerator::addChild(&names, plain);
        }
        if (i + 1 < people.size()) {
            ui::Component comma = ComponentGenerator::createText(key + "Sep", ",\u00a0", "text-sm", "text-gray-400", "", "");
            (*comma.mutable_css())["margin"] = "0";
            ComponentGenerator::addChild(&names, comma);
        }
    }
    ComponentGenerator::addChild(&row, names);
    return row;
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

    // `href`: the value is a link (a source's page), opened in a new tab.
    auto addField = [&](const std::string& idSuffix, const std::string& label, const std::string& value, bool italic,
                        const std::string& href = std::string(), const std::string& color = std::string()) {
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
        if (!color.empty()) {
            (*valueText.mutable_css())["color"] = color;
        }
        if (!href.empty()) {
            ComponentGenerator::addAttribute(&valueText, "data-href", href);
            (*valueText.mutable_css())["color"] = "var(--accent)";
            (*valueText.mutable_css())["text-decoration"] = "underline";
            (*valueText.mutable_css())["overflow-wrap"] = "anywhere";
        }
        ComponentGenerator::addChild(&row, valueText);

        ComponentGenerator::addChild(&panel, row);
    };

    addField("Sentence", "Selected Claim:", claim.sentence(), false);
    debate::ClaimStatus viewerStatus = StepViewMapClaimStatusForUser(claim, viewerUserId, viewerUsername);
    addField("Status", "Status:", StepViewClaimStatusToLabel(viewerStatus), false);
    // Why, for someone who can hold claims; a visitor holds nothing.
    if (viewerUserId > 0 && !anonymous_user::IsAnonymousUsername(viewerUsername)) {
        const StatusExplanation why =
            ExplainStatusForViewer(claimId, fullDebateInfo, collectionProto, userDb, viewerUserId, viewerUsername);
        if (!why.why.empty()) addField("Why", "Why:", why.why, false, std::string(), "var(--text-2)");
        if (!why.conflict.empty()) addField("Conflict", "Conflict:", why.conflict, false, std::string(), "var(--warn)");
    }
    // Who else holds this claim true. Whoever it belongs to is left out: it
    // is theirs, and listing them would say nothing.
    {
        const std::string holder = userDb.getUsername(ClaimVoiceId(claim));
        std::vector<std::pair<std::string, int>> agreed;
        // By name: user_statuses is a protobuf map, unspecified in order.
        for (const auto* entry : ordered::Entries(claim.user_statuses())) {
            if (entry->second != debate::ClaimStatus::TRUE_CLAIM || entry->first == holder) continue;
            agreed.emplace_back(entry->first, userDb.getUserId(entry->first));
        }
        if (!agreed.empty()) {
            ComponentGenerator::addChild(&panel, BuildPeopleField("selectedClaim", "AgreedBy", "Agreed by:", agreed));
        }
    }
    // A name, or "User <id>" for an account that is gone; linked when it is
    // someone's.
    auto person = [&](int userId) -> std::pair<std::string, int> {
        const std::string name = userDb.getUsername(userId);
        if (name.empty()) return {"User " + std::to_string(userId), 0};
        return {name, userId};
    };
    // A claim entered on someone else's behalf shows both: whose claim it is,
    // and who typed it in. One entered by its own author shows one line.
    if (claim.voice_id() > 0 && claim.voice_id() != claim.creator_id()) {
        ComponentGenerator::addChild(&panel, BuildPeopleField("selectedClaim", "Voice", "Said by:",
                                                              {person(claim.voice_id())}));
        ComponentGenerator::addChild(&panel, BuildPeopleField("selectedClaim", "Creator", "Entered by:",
                                                              {person(claim.creator_id())}));
    } else {
        ComponentGenerator::addChild(&panel, BuildPeopleField("selectedClaim", "Creator", "Created by:",
                                                              {person(claim.creator_id())}));
    }
    addField(
        "Description",
        "Description:",
        claim.description().empty() ? "(no description)" : claim.description(),
        claim.description().empty()
    );
    // Where the claim was taken from, when it was. Shown only when there is
    // one: most claims are said directly and a "(no source)" line on each
    // would be noise.
    for (int i = 0; i < claim.sources_size(); ++i) {
        const debate::Source& source = claim.sources(i);
        const std::string suffix = i == 0 ? std::string() : std::to_string(i + 1);
        if (!source.url().empty()) {
            addField("Source" + suffix, "Source:", source.url(), false, source.url());
        }
        if (!source.excerpt().empty()) {
            addField("SourceExcerpt" + suffix, "It says:", "\u201c" + source.excerpt() + "\u201d", true);
        }
        // A picture of the source, under its words, so it can be read at a
        // glance; clicking opens it full size in a new tab.
        if (!source.image_url().empty()) {
            ui::Component image;
            image.set_id("selectedClaimSourceImage" + suffix);
            image.set_type(ui::ComponentType::IMAGE);
            ComponentGenerator::addAttribute(&image, "src", source.image_url());
            image.set_text("A picture of the source");
            ComponentGenerator::addAttribute(&image, "data-href", source.image_url());
            (*image.mutable_css())["display"] = "block";
            (*image.mutable_css())["max-width"] = "100%";
            (*image.mutable_css())["max-height"] = "260px";
            (*image.mutable_css())["object-fit"] = "contain";
            (*image.mutable_css())["border"] = "1px solid var(--line)";
            (*image.mutable_css())["border-radius"] = "6px";
            (*image.mutable_css())["margin"] = "0.25rem 0 0.5rem";
            (*image.mutable_css())["cursor"] = "zoom-in";
            ComponentGenerator::addChild(&panel, image);
        }
    }

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

    // A read-only demo build, and an anonymous visitor, see the claim and
    // none of the buttons that change it: a visitor can read, and signs in
    // (top right) to take part. The server still refuses a visitor's
    // press, so hiding the buttons is presentation, not the guard.
    if (anonymous_user::IsReadOnlyViewer(viewerUsername) || anonymous_user::IsAnonymousUsername(viewerUsername)) {
        ComponentGenerator::addChild(&panel, buttonRow);
        return panel;
    }

    // Who may do what on this claim:
    //   - the viewer HOLDS it (asserted it, it was entered in their voice, or
    //     they agreed with it): they Support it, and if someone challenges it
    //     they answer for themselves. Holding is what the replayed status
    //     says, so a voice's claim that a reader agreed with is the reader's
    //     to defend too. The original holder has no special standing.
    //   - the viewer does not hold it: they Challenge it, or Agree with it.
    //   - the viewer ENTERED it: they may Delete it, whoever holds it.
    const bool viewerHolds = viewerStatus == debate::ClaimStatus::TRUE_CLAIM;
    const bool viewerEntered = viewerUserId > 0 && claim.creator_id() == viewerUserId;
    const bool viewerIsVoice = viewerUserId > 0 && ClaimVoiceId(claim) == viewerUserId;
    // Agreed with it by their own act (the move log), whatever its status is
    // for them now.
    const bool viewerAgreed = !viewerIsVoice &&
        std::find(fullDebateInfo.viewer_held_claim_ids().begin(), fullDebateInfo.viewer_held_claim_ids().end(),
                  claimId) != fullDebateInfo.viewer_held_claim_ids().end();

    // Concede: the selected claim is a challenge, and the viewer holds the
    // claim it challenges.
    int concedeChallengeLinkId = -1;
    for (const auto& link : fullDebateInfo.full_debate_tree().links()) {
        if (link.is_challenge() && link.from_claim_id() == claimId) {
            auto challengedIt = collectionProto.claims_by_id().find(link.to_claim_id());
            if (challengedIt != collectionProto.claims_by_id().end() && viewerUserId > 0 &&
                StepViewMapClaimStatusForUser(challengedIt->second, viewerUserId, viewerUsername) ==
                    debate::ClaimStatus::TRUE_CLAIM) {
                concedeChallengeLinkId = link.link_id();
            }
            break;
        }
    }

    if (viewerHolds) {
        // "Support", not "Add Child": a child claim is evidence offered FOR its
        // parent, so the relationship is what the user is actually creating.
        // The component id stays addChildClaimButton -- it is the wire to the
        // existing event handler, not a label.
        ui::Component addChildButton = ComponentGenerator::createButton(
            "addChildClaimButton",
            "Support",
            "",
            "bg-gray-700",
            "hover:bg-gray-600",
            "text-white",
            "px-3 py-1.5",
            "rounded",
            "text-sm"
        );
        ComponentGenerator::addChild(&buttonRow, addChildButton);

        // A holder who did not make the claim can stop holding it.
        if (!viewerIsVoice) {
            ui::Component withdrawButton = ComponentGenerator::createButton(
                "unendorseClaimButton_" + std::to_string(claimId),
                "Withdraw agreement",
                "",
                "bg-gray-600",
                "hover:bg-gray-700",
                "text-white",
                "px-3 py-1.5",
                "rounded",
                "text-sm"
            );
            ComponentGenerator::addChild(&buttonRow, withdrawButton);
        }
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

        if (viewerAgreed) {
            // They agreed, but something else they hold makes it false for
            // them (see the Conflict line): Agree would do nothing, Withdraw
            // is the way out.
            ComponentGenerator::addChild(&buttonRow, ComponentGenerator::createButton(
                "unendorseClaimButton_" + std::to_string(claimId),
                "Withdraw agreement", "", "bg-gray-600", "hover:bg-gray-700",
                "text-white", "px-3 py-1.5", "rounded", "text-sm"));
        } else {
            ui::Component agreeButton = ComponentGenerator::createButton(
                "endorseClaimButton_" + std::to_string(claimId),
                "Agree",
                "",
                "bg-green-600",
                "hover:bg-green-700",
                "text-white",
                "px-3 py-1.5",
                "rounded",
                "text-sm"
            );
            ComponentGenerator::addChild(&buttonRow, agreeButton);
        }
    }

    if (viewerEntered) {
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
// the graph. Deliberately the claim panel's twin -- same id, same fields in
// the same order, same one Challenge button -- so selecting a pill feels like
// selecting a claim. Only the words differ: what is offered, for what, of
// what kind, and whether it still counts for the viewer.
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
        "border border-gray-700",
        "rounded-lg",
        ""
    );
    (*panel.mutable_css())["display"] = "flex";
    (*panel.mutable_css())["flex-direction"] = "column";
    (*panel.mutable_css())["gap"] = "0.5rem";

    auto relIt = collectionProto.links_by_id().find(relationId);
    if (relIt == collectionProto.links_by_id().end()) {
        ComponentGenerator::addChild(&panel, ComponentGenerator::createText(
            "selectedClaimEmpty", "No argument selected.", "text-sm", "text-gray-400", "", ""));
        return panel;
    }
    const debate::Relationship& rel = relIt->second;

    auto sentenceOf = [&](int claimId) {
        auto it = collectionProto.claims_by_id().find(claimId);
        return it != collectionProto.claims_by_id().end()
            ? it->second.sentence() : "claim " + std::to_string(claimId);
    };
    auto addField = [&](const std::string& idSuffix, const std::string& label,
                        const std::string& value, bool italic) {
        ui::Component row = ComponentGenerator::createContainer(
            "selectedRelation" + idSuffix + "Row", "", "", "", "", "", "", "");
        (*row.mutable_css())["display"] = "flex";
        (*row.mutable_css())["flex-direction"] = "column";
        (*row.mutable_css())["gap"] = "0.15rem";
        ComponentGenerator::addChild(&row, ComponentGenerator::createText(
            "selectedRelation" + idSuffix + "Label", label, "text-xs", "text-gray-500",
            "font-semibold uppercase tracking-wide", ""));
        ui::Component valueText = ComponentGenerator::createText(
            "selectedRelation" + idSuffix + "Value", value, "text-sm", "text-white", "",
            italic ? "italic" : "");
        (*valueText.mutable_css())["overflow-wrap"] = "break-word";
        ComponentGenerator::addChild(&row, valueText);
        ComponentGenerator::addChild(&panel, row);
    };

    // What is offered, and for (or against) what. Reads as one sentence, the
    // way the claim panel's first field reads as the claim.
    std::string offered;
    for (int from : rel.from_claim_ids()) {
        if (!offered.empty()) {
            offered += "  +  ";
        }
        offered += sentenceOf(from);
    }
    const bool opposes = rel.role() == debate::OPPOSES;
    std::string target = rel.to_case() == debate::Relationship::kToClaimId
        ? sentenceOf(rel.to_claim_id())
        : "the argument for: " + sentenceOf(rel.link().connect_to());
    addField("Sentence", "Selected Argument:",
             offered + (opposes ? "  --  against: " : "  --  for: ") + target, false);

    // Status FOR THE VIEWER, in the claim panel's slot. An argument is undercut
    // for whoever holds a challenge against it TRUE; for them it carries
    // nothing. Others' challenges are a count, since they change nothing here.
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
    addField("Status", "Status:",
             undercut ? "Undercut (you hold a challenge against it)"
             : attacks > 0 ? "Challenged (" + std::to_string(attacks) + "), still counts for you"
             : "Standing", false);

    {
        const std::string creatorUsername = userDb.getUsername(rel.creator_id());
        ComponentGenerator::addChild(&panel, BuildPeopleField("selectedClaim", "Creator", "Created by:",
            {creatorUsername.empty() ? std::make_pair("User " + std::to_string(rel.creator_id()), 0)
                                     : std::make_pair(creatorUsername, rel.creator_id())}));
    }

    std::string basis = rel.basis();
    const bool noBasis = basis.empty() || basis == "parent to child connection" || basis == "challenge link";
    addField("Basis", "Kind of reason:", noBasis ? "(none chosen)" : basis, noBasis);

    ui::Component buttonRow = ComponentGenerator::createContainer(
        "selectedRelationButtonRow", "flex gap-2", "", "", "", "", "", "");
    (*buttonRow.mutable_css())["margin-top"] = "0.5rem";
    if (!anonymous_user::IsReadOnlyViewer(viewerUsername) && !anonymous_user::IsAnonymousUsername(viewerUsername)) {
        // The claim panel's Challenge button, for an argument: one click opens
        // the challenge modal with this argument as the target.
        ComponentGenerator::addChild(&buttonRow, ComponentGenerator::createButton(
            "challengeSelectedLinkButton_" + std::to_string(relationId),
            "Challenge", "", "bg-orange-600", "hover:bg-orange-700",
            "text-white", "px-3 py-1.5", "rounded", "text-sm"));
    }
    ComponentGenerator::addChild(&panel, buttonRow);
    return panel;
}

// Open challenges, by person.
//
// A challenge on a claim is a challenge to EVERYONE who holds that claim
// true: whoever asserted it, whoever it was entered in the voice of, and
// whoever agreed with it. Each holder answers for themselves. So the report
// is per holder: the challenges on claims they hold that they have not
// answered. A holder has answered when they no longer hold the claim (they
// conceded, or withdrew their agreement), or when they have challenged the
// challenge back.
//
// A voice never logs in, so its list only shrinks when the reader records
// something the voice actually did -- which is the point: the list shows
// where each person has gone quiet.
struct OpenChallenge {
    int challengeClaimId = 0;
    int targetClaimId = 0;
};

// People by name (case-insensitive); each person's challenges in the order
// the tree's links come, which is relation id, i.e. the order they were made.
using OpenByHolder = std::map<std::string, std::vector<OpenChallenge>, ordered::NameLessFn>;

static OpenByHolder OpenChallengesByHolder(
    const rendering_info::FullDebateViewInfo& fullDebateInfo,
    const debate::Collection& collectionProto,
    VRUserDatabase& userDb
) {
    OpenByHolder open;
    if (!fullDebateInfo.has_full_debate_tree()) return open;
    const auto& tree = fullDebateInfo.full_debate_tree();

    // Whose claim a challenge is, by name: the voice if one was named.
    auto holderOf = [&](int claimId) -> std::string {
        auto it = collectionProto.claims_by_id().find(claimId);
        if (it == collectionProto.claims_by_id().end()) return std::string();
        return userDb.getUsername(ClaimVoiceId(it->second));
    };

    // Who has challenged each claim back, by name.
    std::unordered_map<int, std::set<std::string>> counteredBy;
    for (const auto& link : tree.links()) {
        if (!link.is_challenge()) continue;
        const std::string by = holderOf(link.from_claim_id());
        if (!by.empty()) counteredBy[link.to_claim_id()].insert(by);
    }

    for (const auto& link : tree.links()) {
        if (!link.is_challenge()) continue;
        const int challengeClaimId = link.from_claim_id();
        const int targetClaimId = link.to_claim_id();
        auto targetIt = collectionProto.claims_by_id().find(targetClaimId);
        auto challengeIt = collectionProto.claims_by_id().find(challengeClaimId);
        if (targetIt == collectionProto.claims_by_id().end() ||
            challengeIt == collectionProto.claims_by_id().end()) continue;

        for (const auto& entry : targetIt->second.user_statuses()) {
            const std::string& holder = entry.first;
            if (entry.second != debate::ClaimStatus::TRUE_CLAIM) continue;
            if (holder == holderOf(challengeClaimId)) continue;  // their own challenge
            auto c = counteredBy.find(challengeClaimId);
            if (c != counteredBy.end() && c->second.count(holder)) continue;  // answered
            // A challenge the holder already sees as false is answered too:
            // their own counter, held true, made it so.
            auto view = challengeIt->second.user_statuses().find(holder);
            if (view != challengeIt->second.user_statuses().end() &&
                view->second == debate::ClaimStatus::FALSE_CLAIM) continue;
            open[holder].push_back({challengeClaimId, targetClaimId});
        }
    }
    return open;
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
    (*panel.mutable_css())["gap"] = "0.4rem";
    // A folding section of the inspector, titled by the client from this.
    ComponentGenerator::addAttribute(&panel, "data-section", "Who owes a reply");

    const auto open = OpenChallengesByHolder(fullDebateInfo, collectionProto, userDb);

    if (open.empty()) {
        ui::Component none = ComponentGenerator::createText(
            "debateStatusText", "No open challenges.", "text-sm", "", "", "");
        ComponentGenerator::addChild(&panel, none);
        return panel;
    }

    // The viewer first, then everyone else by name.
    std::vector<std::string> order;
    if (open.count(viewerUsername)) order.push_back(viewerUsername);
    for (const auto& entry : open) {
        if (entry.first != viewerUsername) order.push_back(entry.first);
    }

    for (const std::string& holder : order) {
        const auto& list = open.at(holder);
        const bool isViewer = holder == viewerUsername;
        const std::string who = isViewer ? "You" : holder;
        ui::Component line = ComponentGenerator::createText(
            "debateStatusPerson_" + holder,
            who + ": " + std::to_string(list.size()) + " unanswered" +
                (isViewer ? " (counter-challenge, concede, or withdraw your agreement)" : ""),
            "text-sm",
            "text-white",
            "font-medium",
            ""
        );
        // Red: this person owes a reply (theme.html .owe).
        (*line.mutable_css())["color"] = "var(--open)";
        (*line.mutable_css())["margin-top"] = "0.35rem";
        ComponentGenerator::addChild(&panel, line);

        for (const OpenChallenge& oc : list) {
            std::string sentence;
            auto it = collectionProto.claims_by_id().find(oc.challengeClaimId);
            if (it != collectionProto.claims_by_id().end()) sentence = it->second.sentence();
            ui::Component item = ComponentGenerator::createText(
                "debateStatusChallenge_" + holder + "_" + std::to_string(oc.challengeClaimId),
                sentence,
                "text-xs",
                "text-gray-400",
                "",
                ""
            );
            (*item.mutable_css())["overflow-wrap"] = "break-word";
            (*item.mutable_css())["padding"] = "2px 0 2px 14px";
            (*item.mutable_css())["margin-left"] = "3px";
            (*item.mutable_css())["border-left"] = "2px solid var(--line)";
            ComponentGenerator::addChild(&panel, item);
        }
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

// The claims a line of the AI's text names as [#12] that are claims of this
// debate, comma-separated in order of first mention. The model is told to
// cite claims that way (DebateChat.cc); a number it made up names nothing
// here and stays plain text.
static std::string ClaimLinksIn(const std::string& line, const debate::Collection& collectionProto) {
    std::string out;
    std::set<int> seen;
    for (size_t at = line.find("[#"); at != std::string::npos; at = line.find("[#", at + 2)) {
        size_t end = at + 2;
        while (end < line.size() && end - at < 12 && std::isdigit(static_cast<unsigned char>(line[end]))) ++end;
        if (end == at + 2 || end >= line.size() || line[end] != ']') continue;
        const int claimId = std::stoi(line.substr(at + 2, end - at - 2));
        if (collectionProto.claims_by_id().count(claimId) == 0 || !seen.insert(claimId).second) continue;
        out += (out.empty() ? "" : ",") + std::to_string(claimId);
    }
    return out;
}

// One block of prose as components: one TEXT per line, because the renderer
// collapses whitespace like HTML and a single node would run the model's
// paragraphs together. Given a collection, a line naming its claims as [#12]
// carries data-claim-links, and the client makes those references links.
static void AddTextLines(ui::Component* parent, const std::string& idPrefix, const std::string& text,
                         const std::string& colorClass, const debate::Collection* linkable = nullptr) {
    std::istringstream lines(text);
    std::string line;
    int lineIndex = 0;
    while (std::getline(lines, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const std::string id = idPrefix + "_" + std::to_string(lineIndex++);
        if (line.empty()) {
            ui::Component spacer = ComponentGenerator::createContainer(id, "", "", "", "", "", "", "");
            (*spacer.mutable_css())["height"] = "0.3rem";
            ComponentGenerator::addChild(parent, spacer);
            continue;
        }
        ui::Component textLine = ComponentGenerator::createText(id, line, "text-sm", colorClass, "", "");
        (*textLine.mutable_css())["overflow-wrap"] = "anywhere";
        if (linkable != nullptr) {
            const std::string links = ClaimLinksIn(line, *linkable);
            if (!links.empty()) ComponentGenerator::addAttribute(&textLine, "data-claim-links", links);
        }
        ComponentGenerator::addChild(parent, textLine);
    }
}

// The AI chat. The one place the model speaks: a box for any question, quick
// buttons for the questions readers ask most, and the conversation so far.
// Everything it says is text -- it never writes a claim; the reader does.
//
// It stands beside the argument rather than in it: past a dozen claims the
// graph stops answering "so where does this stand" at a glance, and asking
// should be one click away from wherever the reader is.
ui::Component StepView::BuildChatPanel(const user::User& viewerUser, int selectedClaimId,
                                       const debate::Collection& collectionProto) {
    const user_engagement::DebatingInfo::ChatState& chat = viewerUser.engagement().debating_info().chat();

    // Folded away with one click, so it is always to hand and never in the
    // way. The client owns the fold and the placement.
    ui::Component panel = ComponentGenerator::createContainer(
        "chatPanel", "w-full", "bg-gray-800", "p-3", "", "", "", ""
    );
    ComponentGenerator::addAttribute(&panel, "data-docked", "side");
    ComponentGenerator::addAttribute(&panel, "data-docked-title", "Ask the AI about this debate");

    // The conversation so far, last 20 turns.
    if (chat.messages_size() > 0) {
        ui::Component transcript = ComponentGenerator::createContainer(
            "chatTranscript", "w-full", "", "", "", "", "", ""
        );
        (*transcript.mutable_css())["display"] = "flex";
        (*transcript.mutable_css())["flex-direction"] = "column";
        (*transcript.mutable_css())["gap"] = "0.4rem";
        // No height cap here: the docked panel's body is the one scroller.
        (*transcript.mutable_css())["margin-bottom"] = "0.5rem";

        const int total = chat.messages_size();
        const int first = total > 20 ? total - 20 : 0;
        for (int i = first; i < total; ++i) {
            const auto& m = chat.messages(i);
            const bool mine = m.role() != "assistant";
            ui::Component turn = ComponentGenerator::createContainer(
                "chatTurn_" + std::to_string(i), "w-full", mine ? "" : "bg-gray-900",
                mine ? "" : "p-2", "", "", mine ? "" : "rounded", ""
            );
            (*turn.mutable_css())["display"] = "flex";
            (*turn.mutable_css())["flex-direction"] = "column";
            ui::Component who = ComponentGenerator::createText(
                "chatWho_" + std::to_string(i), mine ? "You" : "AI", "text-xs",
                mine ? "text-indigo-300" : "text-green-300", "font-semibold", ""
            );
            ComponentGenerator::addChild(&turn, who);
            AddTextLines(&turn, "chatLine_" + std::to_string(i), m.text(),
                         mine ? "text-gray-300" : "text-gray-100", mine ? nullptr : &collectionProto);
            ComponentGenerator::addChild(&transcript, turn);
        }
        ComponentGenerator::addChild(&panel, transcript);
    }

    if (chat.pending()) {
        // Marker id ends in Pending so the client polls while an answer is
        // coming -- the client polls on the WORK.
        ui::Component progress = ComponentGenerator::createText(
            "chatPending",
            "Thinking... " + std::to_string(chat.elapsed_seconds()) + "s elapsed",
            "text-sm", "text-indigo-300", "", ""
        );
        ComponentGenerator::addChild(&panel, progress);
        ComponentGenerator::addChild(&panel, ComponentGenerator::createButton(
            "cancelAiWorkButton", "Cancel", "", "bg-gray-700", "hover:bg-gray-600",
            "text-gray-200", "px-3 py-1.5", "rounded", "text-sm"));
        return panel;
    }

    ComponentGenerator::addChild(&panel, ComponentGenerator::createTextarea(
        "chatQuestionInput", "Ask anything about this debate. Ctrl+Enter sends.", "chatQuestion", 3));

    ui::Component buttons = ComponentGenerator::createContainer(
        "chatButtons", "flex gap-2 flex-wrap", "", "", "", "", "", ""
    );
    (*buttons.mutable_css())["display"] = "flex";
    (*buttons.mutable_css())["flex-wrap"] = "wrap";
    (*buttons.mutable_css())["gap"] = "0.5rem";
    ComponentGenerator::addChild(&buttons, ComponentGenerator::createButton(
        "askChatButton", "Ask", "", "bg-indigo-600", "hover:bg-indigo-700",
        "text-white", "px-3 py-1.5", "rounded", "text-sm"));

    // The questions readers ask most, as buttons. The claim ones need a
    // selected claim.
    std::vector<std::string> quick{DebateChat::kSummarizeDebate, DebateChat::kFindMistake};
    if (selectedClaimId > 0) {
        quick.push_back(DebateChat::kSummarizeClaim);
        quick.push_back(DebateChat::kSuggestChallenge);
    }
    for (const std::string& key : quick) {
        ComponentGenerator::addChild(&buttons, ComponentGenerator::createButton(
            "chatQuickButton_" + key, DebateChat::LabelFor(key), "", "bg-gray-700",
            "hover:bg-gray-600", "text-gray-200", "px-3 py-1.5", "rounded-full", "text-sm"));
    }
    if (chat.messages_size() > 0) {
        ComponentGenerator::addChild(&buttons, ComponentGenerator::createButton(
            "clearChatButton", "Clear chat", "", "bg-gray-800", "hover:bg-gray-700",
            "text-gray-400", "px-3 py-1.5", "rounded", "text-sm"));
    }
    ComponentGenerator::addChild(&panel, buttons);

    const user_engagement::AiJobStatus& aiJob = viewerUser.engagement().ai_job();
    if (!aiJob.note().empty()) {
        ComponentGenerator::addChild(&panel, ComponentGenerator::createText(
            "chatNote", aiJob.note(), "text-xs", "text-gray-500", "", ""));
        ComponentGenerator::addChild(&panel, ComponentGenerator::createButton(
            "dismissAiNoticeButton", "Dismiss", "", "bg-gray-700", "hover:bg-gray-600",
            "text-gray-200", "px-3 py-1.5", "rounded", "text-sm"));
    }

    return panel;
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
// The chat as a page's chat region: the window's height beside the debate
// or the brief, its ask box at the bottom (the client lays it out; below
// 1100px it falls back to the corner box).
ui::Component StepView::BuildChatColumn(const user::User& viewerUser, int selectedClaimId,
                                        const debate::Collection& collectionProto) {
    ui::Component column = ComponentGenerator::createContainer("chatColumn", "", "", "", "", "", "", "");
    ComponentGenerator::addAttribute(&column, "data-region", "chat");
    ComponentGenerator::addChild(&column, BuildChatPanel(viewerUser, selectedClaimId, collectionProto));
    return column;
}

// The Debate | Brief tabs. Two buttons that look like tabs: the lit one has
// the accent underline and white text, the other is quiet. Same ids on both
// pages, so the parser has one wire per tab.
ui::Component StepView::BuildDebateTabs(bool briefActive) {
    ui::Component tabs = ComponentGenerator::createContainer("debateTabs", "", "", "", "", "", "", "");
    (*tabs.mutable_css())["display"] = "flex";
    (*tabs.mutable_css())["flex-direction"] = "row";
    (*tabs.mutable_css())["gap"] = "4px";
    (*tabs.mutable_css())["align-items"] = "center";
    ComponentGenerator::addChild(&tabs, BuildTab("debateTabButton", "Debate", !briefActive));
    ComponentGenerator::addChild(&tabs, BuildTab("briefTabButton", "Brief", briefActive));
    return tabs;
}

// One tab, the theme's (theme.html .tab): quiet words, and the lit one on
// the raised surface. The header's Home | Explore tabs are the same.
ui::Component StepView::BuildTab(const std::string& id, const std::string& label, bool lit) {
    ui::Component b = ComponentGenerator::createButton(id, label, "", "", "", "", "", "", "");
    (*b.mutable_css())["background"] = lit ? "var(--panel-2)" : "transparent";
    (*b.mutable_css())["color"] = lit ? "var(--text)" : "var(--text-2)";
    (*b.mutable_css())["border"] = "none";
    (*b.mutable_css())["border-radius"] = "var(--radius-sm)";
    (*b.mutable_css())["padding"] = "6px 12px";
    (*b.mutable_css())["font-size"] = "14px";
    (*b.mutable_css())["font-weight"] = "500";
    return b;
}

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

    // Its height cap rides on a class, not the css map: it is a strip above
    // the page's regions -- see .tutorial-guide-sticky in index.css.
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
	// The page is an app shell: the header, the guide box when there is one,
	// then three regions -- the inspector, the argument, the chat -- that
	// fill the window and scroll on their own. The server says which part is
	// which; the client lays them out (index.css, ContainerComponent).
	ComponentGenerator::addAttribute(&container, "data-layout", "shell");

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
	(*topBar.mutable_css())["justify-content"] = "space-between";
	(*topBar.mutable_css())["align-items"] = "center";
	(*topBar.mutable_css())["flex-wrap"] = "wrap";
	(*topBar.mutable_css())["gap"] = "0.5rem";
	// The tabs sit directly under the site header (LayoutGenerator adds it
	// above everything, with the way home and the account on it).
	ComponentGenerator::addChild(&topBar, BuildDebateTabs(false));
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
	// The inspector: the selected claim pinned on top, folding sections under it.
	ComponentGenerator::addAttribute(&leftColumn, "data-region", "inspector");

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
	// The argument itself, two ways: the map (data-view="graph") and the
	// paragraph tree (data-view="text"). The client shows one at a time.
	ComponentGenerator::addAttribute(&rightColumn, "data-region", "main");


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
	// The selected claim first (the client pins it), then the sections.
	ComponentGenerator::addChild(&leftColumn, selectedClaimPanel);
	ComponentGenerator::addChild(&leftColumn, debateStatusPanel);

	// A single interactive paragraph tree for the entire debate: the Text
	// view of the main region, beside the map.
	ui::Component treeWrapper;
	const bool haveTree = rootClaimId > 0;
	if (haveTree) {
		treeWrapper = ComponentGenerator::createContainer(
			"debateTreeWrapper",
			"w-full",
			"bg-gray-800/50",
			"p-4",
			"",
			"border border-gray-700",
			"rounded-lg",
			""
		);
		ComponentGenerator::addAttribute(&treeWrapper, "data-view", "text");

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
				(*paragraph.mutable_css())["border-top"] = "1px dashed var(--line-strong)";
			}
			ComponentGenerator::addChild(&treeWrapper, paragraph);
		}
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
	(*mapSection.mutable_css())["width"] = "100%";
	// The map fills the main region: its height is the region's, not a
	// minimum of its own (the client sizes it; GraphComponent follows).
	ComponentGenerator::addAttribute(&mapSection, "data-view", "graph");
	ComponentGenerator::addChild(&rightColumn, mapSection);
	if (haveTree) {
		ComponentGenerator::addChild(&rightColumn, treeWrapper);
	}

	ComponentGenerator::addChild(&contentRow, leftColumn);
	ComponentGenerator::addChild(&contentRow, rightColumn);
	// The chat only where a model can be reached (DEBATE_AI, AiAvailability.h).
	if (ai_availability::EnabledFor(viewerUser.username())) {
		ComponentGenerator::addChild(&contentRow, BuildChatColumn(viewerUser, currentClaimId, collectionProto));
	}
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
