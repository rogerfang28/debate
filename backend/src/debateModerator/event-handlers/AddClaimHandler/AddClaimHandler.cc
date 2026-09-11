#include "AddClaimHandler.h"
#include <iostream>
#include "debate.pb.h"
#include "user.pb.h"
#include "../../../utils/pathUtils.h"
#include "../../../utils/Log.h"
#include "../../../utils/BasisRegistry.h"

void AddClaimHandler::AddClaimUnderClaim(const std::vector<std::string>& claim_texts, const std::vector<std::string>& descriptions, const std::string& basis_in, const int& user_id, DebateWrapper& debateWrapper, int voice_id) {
    // A basis the registry cannot explain is not stored: the dropdowns, the
    // prompts and the status engine all read the registry, and a link carrying
    // a name none of them know would be a link nobody can describe or reason
    // about. Unknown becomes unspecified, with a warning.
    const std::string basis = bases::Sanitize(basis_in);
    // first find where the user is in the debate
    // get user protobuf
    user::User userProto = debateWrapper.getUserProtobuf(user_id);
    // std::string debateID = userProto.engagement().debating_info().debate_id();
    int rootClaimID = userProto.engagement().debating_info().root_claim().id();
    user_engagement::ClaimInfo currentClaimInfo = userProto.engagement().debating_info().current_claim();
    int currentClaimID = currentClaimInfo.id();
    int debate_id = userProto.engagement().debating_info().debate_id();

    // DebateWrapper debateWrapper;

    // Several claim texts = ONE relation with several from-claims: "these
    // together support that", not N independent supports. The first row
    // establishes the relation id; the rest join it.
    //
    // Blank entries are skipped rather than becoming empty claims -- the UI
    // offers a fixed number of boxes and most will go unused.
    int relationId = 0;
    int firstClaimId = 0;
    int added = 0;
    // Each claim carries its OWN description, taken at the same index. Indexed
    // against the original position rather than the count added so far, so
    // skipping a blank row cannot shift later descriptions onto other claims.
    for (size_t i = 0; i < claim_texts.size(); ++i) {
        const std::string& text = claim_texts[i];
        const std::string description =
            i < descriptions.size() ? descriptions[i] : std::string();
        if (text.find_first_not_of(" \t\r\n") == std::string::npos) {
            continue;
        }
        const int newClaimID = debateWrapper.addClaimUnderParent(
            currentClaimID,  // parentId
            text,
            description,
            user_id,
            debate_id,
            basis,
            relationId,
            &relationId,
            voice_id
        );
        if (added == 0) {
            firstClaimId = newClaimID;
        }
        ++added;
    }

    if (added == 0) {
        Log::warn("[AddClaimHandler] No non-empty claim text submitted; nothing added.");
    }
    (void)firstClaimId;

    CloseAddChildClaim(user_id, debateWrapper);
}

void AddClaimHandler::CloseAddChildClaim(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set adding_child_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo_CurrentDebateAction::VIEWING_CLAIM);

    debateWrapper.updateUserProtobuf(user_id, userProto);
}

void AddClaimHandler::OpenAddChildClaim(const int& user_id, DebateWrapper& debateWrapper) {
    // get the user from the database
    user::User userProto = debateWrapper.getUserProtobuf(user_id);

    // set adding_child_claim to true
    userProto.mutable_engagement()->mutable_debating_info()->mutable_current_debate_action()->set_action_type(user_engagement::DebatingInfo_CurrentDebateAction::ADDING_CHILD_CLAIM);
    Log::debug("[OpenAddChildClaimHandler] Set adding_child_claim to true for user: " + std::to_string(user_id));
    debateWrapper.updateUserProtobuf(user_id, userProto);
}