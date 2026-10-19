#pragma once

#include <string>

#include "DemoMode.h"
#include "debate_event.pb.h"

// A first-time visitor reads without signing in: the client makes up a name
// like "visitor-a1b2c3d4", keeps it in localStorage, and logs in with it
// silently. Each visitor is its own user record, so two of them never share a
// position (open debate, selected claim, tab). They may read everything and
// change nothing -- the moderator turns their mutating events into NONE.
// They still SEE the buttons: pressing one opens the sign-in modal, and the
// press is remembered and replayed once they have signed in (see
// DebateModerator::handleRequest and VirtualRenderer::handleClientMessage).
//
// "guest" is deliberately NOT anonymous: it is the identity the scenario
// importer and the scenario tests enter data as (the Fauci debates among
// them), so making it read-only would block those writes.
namespace anonymous_user {

inline constexpr const char* kVisitorPrefix = "visitor-";

inline bool IsAnonymousUsername(const std::string& username) {
    return username.rfind(kVisitorPrefix, 0) == 0;
}

// The client-generated shape exactly: "visitor-" plus 8 lowercase hex digits.
// Anything else arriving as a visitor name is ignored.
inline bool IsWellFormedVisitorName(const std::string& username) {
    const std::string prefix = kVisitorPrefix;
    if (username.size() != prefix.size() + 8 || username.rfind(prefix, 0) != 0) {
        return false;
    }
    for (size_t i = prefix.size(); i < username.size(); ++i) {
        const char c = username[i];
        if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
            return false;
        }
    }
    return true;
}

// The one question the layout generators ask: may this viewer see the
// buttons that change a debate? No only in a read-only demo build. An
// anonymous visitor sees them too -- the moderator turns a visitor's press
// into the sign-in modal instead of acting on it -- so the username no longer
// decides; it stays a parameter so the call sites need not care why.
inline bool IsReadOnlyViewer(const std::string& /*username*/) {
    return demo_mode::kReadOnlyMode;
}

// The visitor presses that open the sign-in modal and are remembered, to be
// replayed once the visitor has signed in. Each is the FIRST step of
// something (open a dialog, agree, create): the cancel / close / submit
// steps after it can only come from inside a dialog a visitor never has open,
// so those are simply dropped.
inline bool OpensSignIn(debate_event::EventType type) {
    switch (type) {
        case debate_event::START_CHALLENGE_CLAIM:
        case debate_event::OPEN_ADD_CHALLENGE:
        case debate_event::ADD_CLAIM_TO_BE_CHALLENGED:
        case debate_event::ADD_LINK_TO_BE_CHALLENGED:
        case debate_event::ENDORSE_CLAIM:
        case debate_event::OPEN_ADD_CHILD_CLAIM:
        case debate_event::START_EDIT_CLAIM:
        case debate_event::START_EDIT_CLAIM_DESCRIPTION:
        case debate_event::START_MODIFICATION_OF_CLAIM:
        case debate_event::CREATE_DEBATE:
        case debate_event::GENERATE_DEBATE:
        case debate_event::OPEN_CREATE_DEBATE:
        case debate_event::JOIN_DEBATE:
        case debate_event::START_TUTORIAL:
            return true;
        default:
            return false;
    }
}

// The sign-in modal's title: why it is asking. Plain "Sign in" when it was
// opened from the header (no remembered press).
inline std::string SignInTitle(const std::string& pendingAction) {
    debate_event::DebateEvent pending;
    if (pendingAction.empty() || !pending.ParseFromString(pendingAction)) {
        return "Sign in";
    }
    switch (pending.type()) {
        case debate_event::START_CHALLENGE_CLAIM:
        case debate_event::OPEN_ADD_CHALLENGE:
        case debate_event::ADD_CLAIM_TO_BE_CHALLENGED:
            return "Sign in to challenge this claim";
        case debate_event::ADD_LINK_TO_BE_CHALLENGED:
            return "Sign in to challenge this argument";
        case debate_event::ENDORSE_CLAIM:
            return "Sign in to agree";
        case debate_event::OPEN_ADD_CHILD_CLAIM:
            return "Sign in to support this claim";
        case debate_event::START_EDIT_CLAIM:
        case debate_event::START_EDIT_CLAIM_DESCRIPTION:
        case debate_event::START_MODIFICATION_OF_CLAIM:
            return "Sign in to edit this claim";
        case debate_event::CREATE_DEBATE:
        case debate_event::GENERATE_DEBATE:
        case debate_event::OPEN_CREATE_DEBATE:
            return "Sign in to start a debate";
        case debate_event::JOIN_DEBATE:
            return "Sign in to join this debate";
        case debate_event::START_TUTORIAL:
            return "Sign in to take the walkthrough";
        default:
            return "Sign in";
    }
}

}  // namespace anonymous_user
