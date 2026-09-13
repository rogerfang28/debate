#include "EndorseHandler.h"
#include "../../../utils/Log.h"
#include "user.pb.h"

namespace {

// The claim named, or the one the user stands on. Returns an empty Claim
// (id 0) when there is nothing to act on.
debate::Claim resolveClaim(const int& user_id, int claim_id, DebateWrapper& debateWrapper) {
    if (claim_id <= 0) {
        user::User u = debateWrapper.getUserProtobuf(user_id);
        claim_id = u.engagement().debating_info().current_claim().id();
    }
    return claim_id > 0 ? debateWrapper.getClaimById(claim_id) : debate::Claim();
}

}  // namespace

void EndorseHandler::Endorse(const int& user_id, int claim_id, DebateWrapper& debateWrapper) {
    debate::Claim claim = resolveClaim(user_id, claim_id, debateWrapper);
    const std::string username = debateWrapper.getUserProtobuf(user_id).username();
    if (claim.id() == 0 || username.empty()) {
        Log::warn("[EndorseHandler] Nothing to agree with: claim " + std::to_string(claim_id) +
                  " for user " + std::to_string(user_id));
        return;
    }
    // Nothing is written: the ACCEPT move the logger appends for this event
    // is the record, and the page replays the log.
    Log::debug("[EndorseHandler] " + username + " agrees with claim " + std::to_string(claim.id()));
}

void EndorseHandler::Unendorse(const int& user_id, int claim_id, DebateWrapper& debateWrapper) {
    debate::Claim claim = resolveClaim(user_id, claim_id, debateWrapper);
    const std::string username = debateWrapper.getUserProtobuf(user_id).username();
    if (claim.id() == 0 || username.empty()) {
        Log::warn("[EndorseHandler] Nothing to withdraw: claim " + std::to_string(claim_id) +
                  " for user " + std::to_string(user_id));
        return;
    }
    // Nothing is written: the UNACCEPT move is the record.
    Log::debug("[EndorseHandler] " + username + " withdraws agreement with claim " + std::to_string(claim.id()));
}
