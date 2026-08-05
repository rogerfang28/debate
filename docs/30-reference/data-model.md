---
layer: 3
title: Data Model Reference — SQLite & the Debate Graph
status: partial
last-updated: 2026-08-05
audience: developers (implementation)
source-of-truth-for: [debates.sqlite3 schema, users.sqlite3 schema, graph storage format, MOVES table]
---

# Data Model Reference — SQLite & the Debate Graph

> **Status: partial.** The `MOVES` table below is documented from source. Remaining ⚠️ TODO sections still need filling from actual `.schema` output.

## Databases

| File | Contents |
|---|---|
| `debates.sqlite3` | Debate graphs: claims, links, challenges, resolution states, and the move log |
| `users.sqlite3` | User identity/auth data |

Separation rationale: identity data and debate content have different lifecycles and sensitivity.

## The debate graph

Conceptually (see [`../10-concepts.md`](../10-concepts.md)):

- **Nodes** = claims
- **Edges** = links with type `PARENT_CHILD` or `CHALLENGE` (only these two — see [`protocol.md`](protocol.md))
- Challenges create **nested sub-debates**, so the structure is a recursive graph (tree-like)

### How claims are stored

Claims live in `STATEMENTS`, with the whole `Claim` protobuf serialized into a `STATEMENT_DATA` blob alongside a few real columns (`TEXT`, `CREATOR_ID`, `DEBATE_ID`, `ROOT_ID`). Because the blob *is* the storage format, proto field numbers are part of the schema — see the field-numbering discipline in [`protocol.md`](protocol.md).

Links live in `LINKS` as ordinary typed columns (`CLAIM_ID_FROM`, `CLAIM_ID_TO`, `CONNECTION`, `CREATOR_ID`, `DEBATE_ID`, `LINK_TYPE`) — not as blobs.

### Claims are disconnected, not deleted (by design)

**Deleting a claim never removes its row.** `DebateWrapper::deleteClaim` deletes every link touching the claim and deliberately leaves the `STATEMENTS` row in place — the delete call is commented out in source with that intent stated. The claim becomes *disconnected*: unreachable from the graph, so it vanishes from the UI, while the record of what was said survives.

This is a design decision, not an oversight. Claims are things people asserted; removing them outright would erase the record.

**Deleting a link is real**: `DELETE FROM LINKS WHERE ID = ?`.

Two consequences worth knowing before writing a query or reasoning about the log:

- Any query that does not join through `LINKS` still counts disconnected claims.
- The claim's stored `status` is **not** updated when it is disconnected. Harmless today, since nothing renders a disconnected claim — but it is why the computed and stored models disagree on these claims (see [`backend.md`](backend.md)).

This also aligns with `RETRACT` in the move log, which means "the author withdrew this" and likewise keeps the claim rather than destroying it.

## The move log (`MOVES`)

Append-only record of what people did. Currently a **parallel record only** — nothing reads it to render or decide (see [`backend.md`](backend.md)).

| Column | Notes |
|---|---|
| `ID` | `INTEGER PRIMARY KEY AUTOINCREMENT` |
| `DEBATE_ID` | moves are debate-scoped, not claim-scoped |
| `SEQ` | monotonic **per debate**, assigned server-side |
| `ACTOR_ID` | who acted |
| `TYPE` | text, e.g. `ASSERT` — see `MoveType` in [`protocol.md`](protocol.md) |
| `TARGET_TYPE` | text, `claim` or `relation` |
| `TARGET_ID` | id in whichever table `TARGET_TYPE` names |
| `PAYLOAD` | free-form JSON of structural detail; never claim text |
| `CREATED_AT` | ISO-8601, defaulted by SQLite |

Constraints: `UNIQUE (DEBATE_ID, SEQ)`, plus index `IDX_MOVES_DEBATE_SEQ ON (DEBATE_ID, SEQ)` — replay order for one debate is always `(DEBATE_ID, SEQ)`.

**`TYPE` and `TARGET_TYPE` are text, not integers**, so the log stays readable years later and survives enum renumbering. Unknown names decode to `UNSPECIFIED` rather than being guessed at.

**`SEQ` is assigned inside the INSERT**, via `SELECT COALESCE(MAX(SEQ),0)+1 ... WHERE DEBATE_ID = ?`. A separate read-then-write would leave a window for two writers to claim the same seq, because `Database` serialises individual statements but not statement *pairs*. The `UNIQUE` constraint is the backstop.

⚠️ TODO (fill from source):
- Actual table definitions for `STATEMENTS`, `LINKS`, `DEBATES`, `DEBATE_MEMBERS`
- How challenge lifecycle states (Being Constructed / Open / Resolved) are stored
- Where aggregation shape (independent supports vs. required chain, design case 11) is stored
- Indexes on the non-`MOVES` tables, and any denormalization
- Migration strategy (how schema changes are applied)

## Invariants

- Every edge references existing node IDs.
- Edge `type` ∈ {`PARENT_CHILD`, `CHALLENGE`} — no legacy types.
- Abandoned branches are stored as **abandoned**, never silently as conceded (design case 8), including *which party* went silent.
- **`MOVES` is append-only.** No `UPDATE`, no `DELETE`, ever. A correction is a new move.
- **No status column in `MOVES`.** Status is computed; storing it here would recreate the contradiction the log exists to remove.
- **`SEQ` is gapless and starts at 1 within each debate.** A hole makes replay order ambiguous. Verified by test, and by query: `SELECT DEBATE_ID FROM MOVES GROUP BY DEBATE_ID HAVING COUNT(*) <> MAX(SEQ)` must return nothing.
- **A move write may never break the operation that triggered it.** Failures are logged and dropped while the log runs in parallel.
