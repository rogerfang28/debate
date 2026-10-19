#include <gtest/gtest.h>

#include "../../backend/src/utils/DebateTextFormatter.h"

#include <map>
#include <string>
#include <vector>

// ---------------------------------------
// DebateTextFormatter
// ---------------------------------------
// This is what the model actually reads, so anything it drops is invisible to
// every AI feature built on it -- a summary would simply never mention what was
// lost, and would look complete while doing it. These tests are mostly about
// what has to SURVIVE the flattening from graph to text.

namespace {

debate::Claim MakeClaim(int id, const std::string& sentence, int creatorId) {
    debate::Claim claim;
    claim.set_id(id);
    claim.set_sentence(sentence);
    claim.set_creator_id(creatorId);
    return claim;
}

void AddRelation(debate::Collection& collection, int id, const std::vector<int>& from,
                 int toClaim, debate::RelationRole role, const std::string& basis = "") {
    debate::Relationship rel;
    rel.set_id(id);
    for (int f : from) {
        rel.add_from_claim_ids(f);
    }
    rel.set_to_claim_id(toClaim);
    rel.set_role(role);
    rel.set_basis(basis);
    (*collection.mutable_links_by_id())[id] = rel;
}

DebateTextFormatter::Options ViewerOptions(const std::string& viewer) {
    DebateTextFormatter::Options options;
    options.topic = "Remote work";
    options.viewerUsername = viewer;
    return options;
}

const std::map<int, std::string> kNames = {{1, "alice"}, {2, "bob"}};

} // namespace

TEST(DebateTextFormatterTest, RendersSupportAsIndentationUnderTheClaimItSupports) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "Remote work is productive", 1);
    (*collection.mutable_claims_by_id())[2] = MakeClaim(2, "Commute time is eliminated", 1);
    AddRelation(collection, 10, {2}, 1, debate::SUPPORTS);

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    const size_t root = text.find("Remote work is productive");
    const size_t supportLabel = text.find("SUPPORTED BY");
    const size_t child = text.find("Commute time is eliminated");
    ASSERT_NE(root, std::string::npos);
    ASSERT_NE(supportLabel, std::string::npos);
    ASSERT_NE(child, std::string::npos);
    EXPECT_LT(root, supportLabel) << "the claim must come before what supports it";
    EXPECT_LT(supportLabel, child);
    EXPECT_NE(text.find("by alice"), std::string::npos) << "authorship is part of the argument";
}

// THE DISTINCTION A FLAT LIST DESTROYS. Two claims that jointly license one
// inference are not two independent reasons; a summary that treated them as
// independent would report the argument as more robust than it is.
TEST(DebateTextFormatterTest, JointSupportIsMarkedAsOneArgumentNotSeveral) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "Prices rose", 1);
    (*collection.mutable_claims_by_id())[2] = MakeClaim(2, "Supply fell", 1);
    (*collection.mutable_claims_by_id())[3] = MakeClaim(3, "Demand held steady", 1);
    AddRelation(collection, 10, {2, 3}, 1, debate::SUPPORTS);

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("all of the following TOGETHER"), std::string::npos);
    EXPECT_NE(text.find("Supply fell"), std::string::npos);
    EXPECT_NE(text.find("Demand held steady"), std::string::npos);
}

TEST(DebateTextFormatterTest, SeparateRelationsAreNotMarkedAsJoint) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "Prices rose", 1);
    (*collection.mutable_claims_by_id())[2] = MakeClaim(2, "Supply fell", 1);
    (*collection.mutable_claims_by_id())[3] = MakeClaim(3, "Demand held steady", 1);
    AddRelation(collection, 10, {2}, 1, debate::SUPPORTS);
    AddRelation(collection, 11, {3}, 1, debate::SUPPORTS);

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_EQ(text.find("TOGETHER"), std::string::npos)
        << "independent reasons must not be presented as one joint argument";
}

TEST(DebateTextFormatterTest, ChallengesAreLabelledAsAttacksAndCarryTheirBasis) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "Remote work is productive", 1);
    (*collection.mutable_claims_by_id())[2] = MakeClaim(2, "Output fell after the switch", 2);
    AddRelation(collection, 10, {2}, 1, debate::OPPOSES, "EVIDENTIAL");

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("CHALLENGED BY (evidential)"), std::string::npos);
    EXPECT_NE(text.find("Output fell after the switch"), std::string::npos);
}

// Placeholders from before the basis dropdown existed are not bases, and
// presenting them as one would tell the model somebody chose them.
TEST(DebateTextFormatterTest, LegacyPlaceholderIsNotPresentedAsABasis) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "Root", 1);
    (*collection.mutable_claims_by_id())[2] = MakeClaim(2, "Child", 1);
    AddRelation(collection, 10, {2}, 1, debate::SUPPORTS, "parent to child connection");

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_EQ(text.find("parent to child connection"), std::string::npos);
}

// A challenge on the INFERENCE is a different move from a challenge on a
// claim: it concedes the evidence and denies that it proves the point.
TEST(DebateTextFormatterTest, ChallengeAgainstARelationIsDistinguishedFromOneAgainstAClaim) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "Remote work is productive", 1);
    (*collection.mutable_claims_by_id())[2] = MakeClaim(2, "Commute time is eliminated", 1);
    (*collection.mutable_claims_by_id())[3] = MakeClaim(3, "Saved time is not spent working", 2);
    AddRelation(collection, 10, {2}, 1, debate::SUPPORTS);

    debate::Relationship attack;
    attack.set_id(11);
    attack.add_from_claim_ids(3);
    attack.set_to_relation_id(10);
    attack.set_role(debate::OPPOSES);
    (*collection.mutable_links_by_id())[11] = attack;

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("CHALLENGE AGAINST THE REASONING ABOVE"), std::string::npos);
    EXPECT_NE(text.find("Saved time is not spent working"), std::string::npos);
}

// Disagreement between participants is usually the most important thing on the
// page. Reporting only the viewer's view would hide it entirely.
TEST(DebateTextFormatterTest, DisagreementBetweenParticipantsIsStatedNotAveraged) {
    debate::Collection collection;
    debate::Claim claim = MakeClaim(1, "Remote work is productive", 1);
    (*claim.mutable_user_statuses())["alice"] = debate::TRUE_CLAIM;
    (*claim.mutable_user_statuses())["bob"] = debate::FALSE_CLAIM;
    (*collection.mutable_claims_by_id())[1] = claim;

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("accepted"), std::string::npos) << "the viewer's own standing";
    EXPECT_NE(text.find("DISPUTED"), std::string::npos);
    EXPECT_NE(text.find("bob has it refuted"), std::string::npos);
}

TEST(DebateTextFormatterTest, AgreementIsNotReportedAsDispute) {
    debate::Collection collection;
    debate::Claim claim = MakeClaim(1, "Remote work is productive", 1);
    (*claim.mutable_user_statuses())["alice"] = debate::TRUE_CLAIM;
    (*claim.mutable_user_statuses())["bob"] = debate::TRUE_CLAIM;
    (*collection.mutable_claims_by_id())[1] = claim;

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_EQ(text.find("DISPUTED"), std::string::npos);
}

// A claim the walk never reaches would otherwise vanish, handing the model an
// incomplete debate that looks complete.
TEST(DebateTextFormatterTest, DetachedClaimsAreShownRatherThanDropped) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "Root", 1);
    (*collection.mutable_claims_by_id())[2] = MakeClaim(2, "Child", 1);
    (*collection.mutable_claims_by_id())[3] = MakeClaim(3, "Orphan", 1);
    AddRelation(collection, 10, {2}, 1, debate::SUPPORTS);
    // 3 supports a claim that is not in the collection, so no walk reaches it.
    AddRelation(collection, 11, {3}, 999, debate::SUPPORTS);

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("Orphan"), std::string::npos);
    EXPECT_NE(text.find("NOT CONNECTED TO ANY ROOT"), std::string::npos);
}

// A cycle has no root, so the walk must terminate AND still show the claims.
// This test hanging is precisely the failure it guards against.
TEST(DebateTextFormatterTest, ACycleTerminatesAndStillShowsEveryClaim) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "A", 1);
    (*collection.mutable_claims_by_id())[2] = MakeClaim(2, "B", 1);
    AddRelation(collection, 10, {1}, 2, debate::SUPPORTS);
    AddRelation(collection, 11, {2}, 1, debate::SUPPORTS);

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("[#1] A"), std::string::npos);
    EXPECT_NE(text.find("[#2] B"), std::string::npos);
}

TEST(DebateTextFormatterTest, AClaimSupportingTwoParentsIsNotExpandedTwice) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "First parent", 1);
    (*collection.mutable_claims_by_id())[2] = MakeClaim(2, "Second parent", 1);
    (*collection.mutable_claims_by_id())[3] = MakeClaim(3, "Shared evidence", 1);
    AddRelation(collection, 10, {3}, 1, debate::SUPPORTS);
    AddRelation(collection, 11, {3}, 2, debate::SUPPORTS);

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("already shown above"), std::string::npos);
}

TEST(DebateTextFormatterTest, UnknownAuthorFallsBackToAnIdRatherThanBlank) {
    debate::Collection collection;
    (*collection.mutable_claims_by_id())[1] = MakeClaim(1, "Root", 77);

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("by User 77"), std::string::npos);
}

TEST(DebateTextFormatterTest, AnEmptyDebateSaysSoInsteadOfProducingAnEmptyPrompt) {
    debate::Collection collection;

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("no claims yet"), std::string::npos);
}

TEST(DebateTextFormatterTest, DescriptionsAreIncludedBecauseTheyCarryTheActualEvidence) {
    debate::Collection collection;
    debate::Claim claim = MakeClaim(1, "Commute time is eliminated", 1);
    claim.set_description("Average commute is 54 minutes daily.");
    (*collection.mutable_claims_by_id())[1] = claim;

    const std::string text = DebateTextFormatter::Format(collection, kNames, ViewerOptions("alice"));

    EXPECT_NE(text.find("Average commute is 54 minutes daily."), std::string::npos);
}
