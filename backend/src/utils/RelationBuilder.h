#pragma once

#include "debate.pb.h"

#include <string>

// ---------------------------------------
// RelationBuilder
// ---------------------------------------
// The ONE place a stored link row becomes a debate::Relationship.
//
// Every relation in memory is built here, so the new hyperedge shape and the
// legacy Link sub-message cannot drift apart. Before this existed the same
// six set_ calls were copy-pasted in three places, which is precisely how a
// field ends up populated on two paths out of three.
//
// MIGRATION SHAPE. This fills BOTH representations from the same row:
//
//   legacy  -- Relationship.link, a single connect_from -> connect_to pair
//   new     -- from_claim_ids (a LIST) -> to_claim_id, plus role and basis
//
// Writing both is what makes the migration survivable. Readers move to the new
// fields one at a time and can be verified individually; nothing has to change
// in lockstep, and the app keeps running throughout. When no reader touches
// `link` any more, it gets deleted and this function stops filling it.
//
// TODAY from_claim_ids ALWAYS HAS EXACTLY ONE ENTRY, because the LINKS table
// has a single CLAIM_ID_FROM column and structurally cannot hold more. The list
// is not pretending to be general -- it is the shape callers should already be
// written against, so that widening the storage later is a change to this
// function and the schema, not to every consumer.
namespace RelationBuilder {

// linkType is debate::LinkType as stored (an int column).
//
// PARENT_CHILD and NORMAL map to SUPPORTS, CHALLENGE maps to OPPOSES. That
// mapping is the one stated in debate.proto next to the legacy enum.
//
// `connection` becomes `basis`. It holds a real basis (EVIDENTIAL, AUTHORITY,
// ...) for anything created since the dropdown landed, and the old placeholder
// string "parent to child connection" for everything before it. Rather than
// launder that placeholder into a basis it never was, it is carried through
// as-is -- a caller reading `basis` can tell an unset one from a chosen one,
// which it could not if this silently substituted a default.
debate::Relationship fromLinkRow(int linkId,
                                 int fromClaimId,
                                 int toClaimId,
                                 const std::string& connection,
                                 int creatorId,
                                 int linkType,
                                 int debateId = 0);

}  // namespace RelationBuilder
