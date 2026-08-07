#include <gtest/gtest.h>

#include "../../backend/src/utils/RelationBuilder.h"

// ---------------------------------------
// RelationBuilder
// ---------------------------------------
// Guards the migration invariant: every relation carries BOTH shapes, and they
// describe the same edge. While readers are being moved across one at a time,
// a producer that filled only one shape would give two callers different
// answers about the same row -- the exact failure this file exists to prevent.

TEST(RelationBuilderTest, ParentChildFillsBothShapesConsistently) {
    const debate::Relationship rel =
        RelationBuilder::fromLinkRow(7, 3, 9, "EVIDENTIAL", 42,
                                     static_cast<int>(debate::LinkType::PARENT_CHILD), 5);

    // Legacy shape, exactly as before.
    EXPECT_EQ(rel.link().id(), 7);
    EXPECT_EQ(rel.link().connect_from(), 3);
    EXPECT_EQ(rel.link().connect_to(), 9);
    EXPECT_EQ(rel.link().connection(), "EVIDENTIAL");
    EXPECT_EQ(rel.link().creator_id(), 42);
    EXPECT_EQ(rel.link().link_type(), debate::LinkType::PARENT_CHILD);
    EXPECT_EQ(rel.link().debate_id(), 5);

    // New shape, describing the same edge.
    EXPECT_EQ(rel.id(), 7);
    EXPECT_EQ(rel.debate_id(), 5);
    ASSERT_EQ(rel.from_claim_ids_size(), 1)
        << "storage has one CLAIM_ID_FROM column, so the list holds exactly one";
    EXPECT_EQ(rel.from_claim_ids(0), 3);
    EXPECT_EQ(rel.to_case(), debate::Relationship::kToClaimId);
    EXPECT_EQ(rel.to_claim_id(), 9);
    EXPECT_EQ(rel.creator_id(), 42);
    EXPECT_EQ(rel.basis(), "EVIDENTIAL");
    EXPECT_EQ(rel.role(), debate::SUPPORTS);
}

TEST(RelationBuilderTest, ChallengeBecomesOpposes) {
    const debate::Relationship rel =
        RelationBuilder::fromLinkRow(11, 4, 2, "", 1,
                                     static_cast<int>(debate::LinkType::CHALLENGE), 5);

    EXPECT_EQ(rel.link().link_type(), debate::LinkType::CHALLENGE);
    EXPECT_EQ(rel.role(), debate::OPPOSES)
        << "CHALLENGE is the only link type that maps to OPPOSES";
    ASSERT_EQ(rel.from_claim_ids_size(), 1);
    EXPECT_EQ(rel.from_claim_ids(0), 4);
    EXPECT_EQ(rel.to_claim_id(), 2);
}

// NORMAL carries no agreed meaning in the old model and nothing computes from
// it, so folding it in with PARENT_CHILD is the reading that changes no
// behaviour. Pinned so that choice is deliberate rather than incidental.
TEST(RelationBuilderTest, NormalFoldsIntoSupports) {
    const debate::Relationship rel =
        RelationBuilder::fromLinkRow(2, 1, 2, "", 1,
                                     static_cast<int>(debate::LinkType::NORMAL), 5);
    EXPECT_EQ(rel.role(), debate::SUPPORTS);
}

// The pre-dropdown placeholder is carried through rather than laundered into a
// real basis. A reader has to be able to tell "nobody chose" from "someone
// chose EVIDENTIAL", and substituting a default here would erase that.
TEST(RelationBuilderTest, LegacyPlaceholderConnectionIsNotLaunderedIntoABasis) {
    const debate::Relationship rel =
        RelationBuilder::fromLinkRow(1, 1, 2, "parent to child connection", 1,
                                     static_cast<int>(debate::LinkType::PARENT_CHILD), 5);

    EXPECT_EQ(rel.basis(), "parent to child connection");
    EXPECT_NE(rel.basis(), "EVIDENTIAL");
}

// A relation whose target is another relation is the golden rule (R3). Nothing
// produces one yet, but the oneof must be able to express it, and reading
// to_claim_id on such a relation must NOT silently answer 0 as though it
// targeted a claim.
TEST(RelationBuilderTest, RelationTargetedRelationIsDistinguishable) {
    debate::Relationship rel;
    rel.add_from_claim_ids(3);
    rel.set_to_relation_id(8);

    EXPECT_EQ(rel.to_case(), debate::Relationship::kToRelationId);
    EXPECT_NE(rel.to_case(), debate::Relationship::kToClaimId)
        << "callers must branch on to_case(), never assume a claim target";
}
