#include <gtest/gtest.h>

#include "../../backend/src/utils/RelationBuilder.h"

// ---------------------------------------
// RelationBuilder
// ---------------------------------------
// Guards the migration invariant: every relation carries BOTH shapes, and they
// describe the same edge. While readers are being moved across one at a time,
// a producer that filled only one shape would give two callers different
// answers about the same row -- the exact failure this file exists to prevent.

// PARENT_CHILD SWAPS ENDS. The row runs parent -> child, but the child is
// evidence offered FOR the parent, so the relation runs child -> parent. This
// is the single easiest thing to get backwards in the migration, and getting it
// backwards renders the tree upside down.
TEST(RelationBuilderTest, ParentChildSwapsEndsSoTheChildIsTheSupporter) {
    // Row: parent 3 -> child 9.
    const debate::Relationship rel =
        RelationBuilder::fromLinkRow(7, /*parent=*/3, /*child=*/9, "EVIDENTIAL", 42,
                                     static_cast<int>(debate::LinkType::PARENT_CHILD), 5);

    // Legacy shape stays EXACTLY as stored -- unmigrated readers must not move.
    EXPECT_EQ(rel.link().id(), 7);
    EXPECT_EQ(rel.link().connect_from(), 3) << "still the parent, as stored";
    EXPECT_EQ(rel.link().connect_to(), 9) << "still the child, as stored";
    EXPECT_EQ(rel.link().link_type(), debate::LinkType::PARENT_CHILD);
    EXPECT_EQ(rel.link().debate_id(), 5);

    // New shape is SEMANTIC, and therefore reversed.
    EXPECT_EQ(rel.id(), 7);
    EXPECT_EQ(rel.debate_id(), 5);
    ASSERT_EQ(rel.from_claim_ids_size(), 1)
        << "storage has one CLAIM_ID_FROM column, so the list holds exactly one";
    EXPECT_EQ(rel.from_claim_ids(0), 9) << "the CHILD supports";
    EXPECT_EQ(rel.to_claim_id(), 3) << "the PARENT is supported";
    EXPECT_EQ(rel.to_case(), debate::Relationship::kToClaimId);
    EXPECT_EQ(rel.role(), debate::SUPPORTS);
    EXPECT_EQ(rel.creator_id(), 42);
    EXPECT_EQ(rel.basis(), "EVIDENTIAL");

    // Stated as its own assertion so a regression names the actual mistake.
    EXPECT_NE(rel.from_claim_ids(0), rel.link().connect_from())
        << "a parent-child relation must NOT copy the row's ends straight across";
}

// CHALLENGE does NOT swap. The row already runs challenger -> challenged, which
// is the direction of attack. Paired with the test above: the two link types
// are handled differently on purpose, so neither can be "fixed" to match the
// other without breaking the one that was right.
TEST(RelationBuilderTest, ChallengeKeepsItsDirection) {
    const debate::Relationship rel =
        RelationBuilder::fromLinkRow(11, /*challenger=*/4, /*challenged=*/2, "", 1,
                                     static_cast<int>(debate::LinkType::CHALLENGE), 5);

    EXPECT_EQ(rel.link().link_type(), debate::LinkType::CHALLENGE);
    EXPECT_EQ(rel.role(), debate::OPPOSES);
    ASSERT_EQ(rel.from_claim_ids_size(), 1);
    EXPECT_EQ(rel.from_claim_ids(0), 4) << "the challenger attacks";
    EXPECT_EQ(rel.to_claim_id(), 2) << "the challenged is attacked";

    EXPECT_EQ(rel.from_claim_ids(0), rel.link().connect_from())
        << "a challenge relation DOES copy the row's ends straight across";
}

// NORMAL must NOT become SUPPORTS.
//
// Every reader today is `if (PARENT_CHILD) ... else if (CHALLENGE) ... else
// continue`, so NORMAL links are dropped: not drawn, not propagated. Mapping
// NORMAL to SUPPORTS would destroy that distinction before the reader sees it,
// and those links would abruptly start rendering and start carrying status --
// a silent behaviour change in the middle of a refactor whose entire safety
// check is "nothing on screen moved".
TEST(RelationBuilderTest, NormalStaysUnspecifiedSoReadersKeepDroppingIt) {
    const debate::Relationship rel =
        RelationBuilder::fromLinkRow(2, 1, 2, "", 1,
                                     static_cast<int>(debate::LinkType::NORMAL), 5);

    EXPECT_EQ(rel.role(), debate::ROLE_UNSPECIFIED);
    EXPECT_NE(rel.role(), debate::SUPPORTS)
        << "NORMAL links are invisible today; SUPPORTS would make them appear";
    EXPECT_NE(rel.role(), debate::OPPOSES);
}

// Three link types must survive as three roles. If any two collapse, a reader
// branching on role() cannot reproduce what a reader branching on link_type()
// does today, and no downstream branching can recover the lost distinction.
TEST(RelationBuilderTest, ThreeLinkTypesStayThreeDistinctRoles) {
    auto roleOf = [](debate::LinkType t) {
        return RelationBuilder::fromLinkRow(1, 1, 2, "", 1, static_cast<int>(t), 5).role();
    };

    EXPECT_NE(roleOf(debate::LinkType::PARENT_CHILD), roleOf(debate::LinkType::CHALLENGE));
    EXPECT_NE(roleOf(debate::LinkType::PARENT_CHILD), roleOf(debate::LinkType::NORMAL));
    EXPECT_NE(roleOf(debate::LinkType::CHALLENGE), roleOf(debate::LinkType::NORMAL));
}

// from_claim_ids(0) on an empty repeated field is undefined behaviour, not an
// exception -- and every consumer reaches for index 0. So a relation missing an
// endpoint must never be built at all.
TEST(RelationBuilderTest, RefusesToBuildARelationMissingAnEndpoint) {
    for (auto [from, to] : {std::pair{0, 5}, std::pair{5, 0}, std::pair{0, 0},
                            std::pair{-1, 5}}) {
        const debate::Relationship rel = RelationBuilder::fromLinkRow(
            9, from, to, "", 1, static_cast<int>(debate::LinkType::PARENT_CHILD), 5);

        EXPECT_EQ(rel.from_claim_ids_size(), 0)
            << "from=" << from << " to=" << to << " must not produce a from-set";
        EXPECT_EQ(rel.id(), 0) << "callers detect this the same way they detect a missing row";
        EXPECT_EQ(rel.link().id(), 0) << "the legacy shape must be empty too, not half-built";
    }
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
