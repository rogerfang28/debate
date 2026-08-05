// test_status_engine.cc — the pure status function
//
// computeStatuses takes a graph and a log and returns statuses. No database,
// no server, no fixture: every case here is built inline, which is the whole
// point of keeping it pure.
//
// Scope matches the function: ASSERT, OPPOSE, CONCEDE, RETRACT producing
// COLLAPSED and STANDING. Cases for the other nine statuses belong here only
// once the moves that justify them are actually logged.

#include <gtest/gtest.h>
#include <vector>

#include "debate.pb.h"
#include "statusEngine/StatusEngine.h"

namespace {

debate::Claim claim(int id) {
    debate::Claim c;
    c.set_id(id);
    c.set_sentence("claim " + std::to_string(id));
    return c;
}

// A legacy PARENT_CHILD link: stored parent -> child, meaning child supports
// parent. Collapse therefore travels child -> parent.
debate::Relationship parentChild(int linkId, int parent, int child) {
    debate::Relationship r;
    auto* l = r.mutable_link();
    l->set_id(linkId);
    l->set_connect_from(parent);
    l->set_connect_to(child);
    l->set_link_type(debate::LinkType::PARENT_CHILD);
    return r;
}

debate::Relationship challengeLink(int linkId, int challenger, int challenged) {
    debate::Relationship r;
    auto* l = r.mutable_link();
    l->set_id(linkId);
    l->set_connect_from(challenger);
    l->set_connect_to(challenged);
    l->set_link_type(debate::LinkType::CHALLENGE);
    return r;
}

debate::Move move(int seq, debate::MoveType type, int targetClaim) {
    debate::Move m;
    m.set_seq(seq);
    m.set_type(type);
    m.set_target_type(debate::TARGET_CLAIM);
    m.set_target_id(targetClaim);
    return m;
}

debate::Status statusOf(const StatusEngine::Result& r, int claimId) {
    auto it = r.claim_status.find(claimId);
    return it == r.claim_status.end() ? debate::STATUS_UNSPECIFIED : it->second;
}

}  // namespace

TEST(StatusEngineTest, UnattackedClaimsAreStanding) {
    // The burden rule: an unattacked claim stands, bare assertion included.
    // Not "verified" -- just not yet knocked down.
    std::vector<debate::Claim> claims{claim(1), claim(2)};
    std::vector<debate::Relationship> rels{parentChild(10, 1, 2)};
    std::vector<debate::Move> moves{move(1, debate::ASSERT, 1),
                                    move(2, debate::ASSERT, 2)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(r.claim_status.size(), 2u);
    EXPECT_EQ(statusOf(r, 1), debate::STANDING);
    EXPECT_EQ(statusOf(r, 2), debate::STANDING);
}

TEST(StatusEngineTest, ConcedeCollapsesTheClaim) {
    std::vector<debate::Claim> claims{claim(1)};
    std::vector<debate::Move> moves{move(1, debate::ASSERT, 1),
                                    move(2, debate::CONCEDE, 1)};

    auto r = StatusEngine::computeStatuses(claims, {}, moves);
    EXPECT_EQ(statusOf(r, 1), debate::COLLAPSED);
}

TEST(StatusEngineTest, RetractCollapsesTheClaim) {
    // RETRACT and CONCEDE mean different things to a reader -- "I am taking
    // this back" versus "you defeated this" -- but both remove the claim from
    // play, so both land on COLLAPSED here.
    std::vector<debate::Claim> claims{claim(1)};
    std::vector<debate::Move> moves{move(1, debate::ASSERT, 1),
                                    move(2, debate::RETRACT, 1)};

    auto r = StatusEngine::computeStatuses(claims, {}, moves);
    EXPECT_EQ(statusOf(r, 1), debate::COLLAPSED);
}

TEST(StatusEngineTest, CollapseCascadesFromChildToParent) {
    // Claim 2 is evidence offered for claim 1. Knock out the evidence and the
    // claim resting on it goes too. The stored link runs parent -> child, so
    // the cascade runs against the stored direction -- the easiest thing here
    // to get backwards.
    std::vector<debate::Claim> claims{claim(1), claim(2)};
    std::vector<debate::Relationship> rels{parentChild(10, 1, 2)};
    std::vector<debate::Move> moves{move(1, debate::CONCEDE, 2)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(statusOf(r, 2), debate::COLLAPSED) << "the conceded claim";
    EXPECT_EQ(statusOf(r, 1), debate::COLLAPSED) << "and what rested on it";
}

TEST(StatusEngineTest, CascadeTravelsMultipleLevels) {
    // 3 supports 2 supports 1.
    std::vector<debate::Claim> claims{claim(1), claim(2), claim(3)};
    std::vector<debate::Relationship> rels{parentChild(10, 1, 2),
                                           parentChild(11, 2, 3)};
    std::vector<debate::Move> moves{move(1, debate::CONCEDE, 3)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(statusOf(r, 3), debate::COLLAPSED);
    EXPECT_EQ(statusOf(r, 2), debate::COLLAPSED);
    EXPECT_EQ(statusOf(r, 1), debate::COLLAPSED);
}

TEST(StatusEngineTest, CascadeDoesNotTravelDownwards) {
    // Conceding the parent must not take its evidence with it. The evidence
    // was never disputed and may support something else entirely.
    std::vector<debate::Claim> claims{claim(1), claim(2)};
    std::vector<debate::Relationship> rels{parentChild(10, 1, 2)};
    std::vector<debate::Move> moves{move(1, debate::CONCEDE, 1)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(statusOf(r, 1), debate::COLLAPSED);
    EXPECT_EQ(statusOf(r, 2), debate::STANDING) << "evidence survives its conclusion";
}

TEST(StatusEngineTest, OpposeMakesTheTargetOpenNotDefeated) {
    // An attack has no mechanical success condition. It bites only when
    // someone concedes or times out. So a challenge marks its target as under
    // dispute -- it does not defeat it. Treating an unanswered OPPOSE as a
    // defeat would be the tool declaring a winner, which it must never do.
    std::vector<debate::Claim> claims{claim(1), claim(2)};
    std::vector<debate::Relationship> rels{challengeLink(10, 2, 1)};
    std::vector<debate::Move> moves{move(1, debate::OPPOSE, 1)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(statusOf(r, 1), debate::OPEN) << "challenged, not defeated";
    EXPECT_EQ(statusOf(r, 2), debate::STANDING) << "the challenge itself stands";
}

TEST(StatusEngineTest, CollapsedOutranksOpen) {
    // A claim can be challenged AND conceded. Settled beats disputed: the
    // precedence ladder puts COLLAPSED above OPEN.
    std::vector<debate::Claim> claims{claim(1), claim(2)};
    std::vector<debate::Relationship> rels{challengeLink(10, 2, 1)};
    std::vector<debate::Move> moves{move(1, debate::OPPOSE, 1),
                                    move(2, debate::CONCEDE, 1)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(statusOf(r, 1), debate::COLLAPSED);
}

TEST(StatusEngineTest, WithdrawnChallengeLetsTheTargetRecover) {
    // The challenger takes their challenge back. The attack stops holding the
    // target open -- you cannot leave a claim looking disputed by attacking it
    // and then walking away.
    std::vector<debate::Claim> claims{claim(1), claim(2)};
    std::vector<debate::Relationship> rels{challengeLink(10, 2, 1)};
    std::vector<debate::Move> moves{move(1, debate::OPPOSE, 1),
                                    move(2, debate::RETRACT, 2)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(statusOf(r, 2), debate::COLLAPSED) << "the withdrawn challenge";
    EXPECT_EQ(statusOf(r, 1), debate::STANDING) << "its target recovers";
}

TEST(StatusEngineTest, OneLiveChallengeAmongSeveralKeepsTheTargetOpen) {
    // Two challengers, one withdraws. The other still holds the claim open.
    std::vector<debate::Claim> claims{claim(1), claim(2), claim(3)};
    std::vector<debate::Relationship> rels{challengeLink(10, 2, 1),
                                           challengeLink(11, 3, 1)};
    std::vector<debate::Move> moves{move(1, debate::OPPOSE, 1),
                                    move(2, debate::OPPOSE, 1),
                                    move(3, debate::RETRACT, 2)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(statusOf(r, 1), debate::OPEN);
    EXPECT_EQ(statusOf(r, 2), debate::COLLAPSED);
    EXPECT_EQ(statusOf(r, 3), debate::STANDING);
}

TEST(StatusEngineTest, CollapsedChallengeDoesNotDragDownItsTarget) {
    // A CHALLENGE link is opposition, not support, so collapse must not travel
    // along it. If it did, giving up your own challenge would destroy the
    // claim you were attacking.
    std::vector<debate::Claim> claims{claim(1), claim(2)};
    std::vector<debate::Relationship> rels{challengeLink(10, 2, 1)};
    std::vector<debate::Move> moves{move(1, debate::CONCEDE, 2)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(statusOf(r, 2), debate::COLLAPSED) << "the challenge itself";
    EXPECT_EQ(statusOf(r, 1), debate::STANDING) << "the claim it attacked is untouched";
}

TEST(StatusEngineTest, SupportCyclesTerminate) {
    // Users can create loops, so cycles must be handled rather than assumed
    // away. 1 <- 2 <- 3 <- 1.
    std::vector<debate::Claim> claims{claim(1), claim(2), claim(3)};
    std::vector<debate::Relationship> rels{parentChild(10, 1, 2),
                                           parentChild(11, 2, 3),
                                           parentChild(12, 3, 1)};
    std::vector<debate::Move> moves{move(1, debate::CONCEDE, 1)};

    auto r = StatusEngine::computeStatuses(claims, rels, moves);
    // Everything in the loop collapses, and crucially the call returns.
    EXPECT_EQ(statusOf(r, 1), debate::COLLAPSED);
    EXPECT_EQ(statusOf(r, 2), debate::COLLAPSED);
    EXPECT_EQ(statusOf(r, 3), debate::COLLAPSED);
}

TEST(StatusEngineTest, UnsortedMovesGiveTheSameAnswer) {
    std::vector<debate::Claim> claims{claim(1), claim(2)};
    std::vector<debate::Relationship> rels{parentChild(10, 1, 2)};

    std::vector<debate::Move> inOrder{move(1, debate::ASSERT, 2),
                                      move(2, debate::CONCEDE, 2)};
    std::vector<debate::Move> shuffled{move(2, debate::CONCEDE, 2),
                                       move(1, debate::ASSERT, 2)};

    auto a = StatusEngine::computeStatuses(claims, rels, inOrder);
    auto b = StatusEngine::computeStatuses(claims, rels, shuffled);
    EXPECT_EQ(a.claim_status, b.claim_status);
}

TEST(StatusEngineTest, IsPureAndRepeatable) {
    // Same inputs, same outputs, and the inputs are not modified.
    std::vector<debate::Claim> claims{claim(1), claim(2)};
    std::vector<debate::Relationship> rels{parentChild(10, 1, 2)};
    std::vector<debate::Move> moves{move(1, debate::CONCEDE, 2)};

    auto first = StatusEngine::computeStatuses(claims, rels, moves);
    auto second = StatusEngine::computeStatuses(claims, rels, moves);
    EXPECT_EQ(first.claim_status, second.claim_status);
    EXPECT_EQ(claims.size(), 2u);
    EXPECT_EQ(moves.size(), 1u);
}

TEST(StatusEngineTest, IgnoresStoredStatusEntirely) {
    // The old model's fields must have no influence. A claim marked
    // FALSE_CLAIM in storage but never conceded is STANDING here -- that
    // divergence is the entire reason this function exists.
    debate::Claim c = claim(1);
    c.set_status(debate::ClaimStatus::FALSE_CLAIM);
    (*c.mutable_user_statuses())["alice"] = debate::ClaimStatus::FALSE_CLAIM;

    auto r = StatusEngine::computeStatuses({c}, {}, {});
    EXPECT_EQ(statusOf(r, 1), debate::STANDING)
        << "stored status must not leak into the computed answer";
}

TEST(StatusEngineTest, HandlesTheNewHyperedgeShape) {
    // Not yet written by anything, but the function must already read it:
    // both members of the from-set support claim 3, so collapsing either
    // collapses 3.
    std::vector<debate::Claim> claims{claim(1), claim(2), claim(3)};

    debate::Relationship rel;
    rel.set_id(50);
    rel.add_from_claim_ids(1);
    rel.add_from_claim_ids(2);
    rel.set_to_claim_id(3);
    rel.set_role(debate::SUPPORTS);

    std::vector<debate::Move> moves{move(1, debate::CONCEDE, 1)};

    auto r = StatusEngine::computeStatuses(claims, {rel}, moves);
    EXPECT_EQ(statusOf(r, 1), debate::COLLAPSED);
    EXPECT_EQ(statusOf(r, 3), debate::COLLAPSED) << "the supported claim";
    EXPECT_EQ(statusOf(r, 2), debate::STANDING) << "the other member is untouched";
}
