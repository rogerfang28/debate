---
layer: 3
title: Data Model Reference — SQLite & the Debate Graph
status: partial
last-updated: 2026-08-05
audience: developers (implementation)
source-of-truth-for: [debates.sqlite3 schema, users.sqlite3 schema, graph storage format]
---

# Data Model Reference — SQLite & the Debate Graph

> **Status: partial.** Fill ⚠️ TODO sections from the actual schema (`.schema` output) when working here.

## Databases

| File | Contents |
|---|---|
| `debates.sqlite3` | Debate graphs: claims, links, challenges, resolution states |
| `users.sqlite3` | User identity/auth data |

Separation rationale: identity data and debate content have different lifecycles and sensitivity.

## The debate graph

Conceptually (see [`../10-concepts.md`](../10-concepts.md)):

- **Nodes** = claims
- **Edges** = links with type `PARENT_CHILD` or `CHALLENGE` (only these two — see [`protocol.md`](protocol.md))
- Challenges create **nested sub-debates**, so the structure is a recursive graph (tree-like)

⚠️ TODO (fill from source):
- Actual table definitions (claims/nodes, links/edges, challenges, debates, resolution state columns)
- How challenge lifecycle states (Being Constructed / Open / Resolved) are stored
- How concession cascades are persisted (recomputed on read vs. materialized on write?)
- Where aggregation shape (independent supports vs. required chain, design case 11) is stored
- Indexes and any denormalization
- Migration strategy (how schema changes are applied)

## Invariants

- Every edge references existing node IDs.
- Edge `type` ∈ {`PARENT_CHILD`, `CHALLENGE`} — no legacy types.
- Abandoned branches are stored as **abandoned**, never silently as conceded (design case 8), including *which party* went silent.
