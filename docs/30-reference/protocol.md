---
layer: 3
title: Protocol Reference — Protocol Buffers Contract
status: partial
last-updated: 2026-08-05
audience: developers (implementation)
source-of-truth-for: [debate.proto, LinkType enum, RelationRole enum, MoveType enum, Status enum, client/server message envelopes, proto generation]
---

# Protocol Reference — Protocol Buffers

> **Status: partial.** The enum contracts below are authoritative; message envelopes are still ⚠️ TODO.

All communication between frontend and backend — including UI layouts — is Protocol Buffers, carried over a single `POST /clientmessage` endpoint.

## Two shapes coexist

`debate.proto` currently carries **both** a legacy shape and a newer one. This is deliberate and temporary: the legacy shape is what every code path reads and writes today, and the new shape is additive so that nothing breaks while it is adopted.

| Shape | Where | Written today? |
|---|---|---|
| Legacy — `Relationship.link`, one source → one target claim, typed by `LinkType` | field 1 of `Relationship` | **yes**, by all current code |
| New — a from-**set**, a target that may be a claim *or another relation*, plus `role` and `basis` | fields 2+ of `Relationship` | **no**, nothing populates it yet |

A legacy link is the single-source instance of the general case, so the two are not in conflict. When `Relationship.link` is retired, field 1 becomes `reserved`.

## `LinkType` enum (canonical for persisted data — do not drift)

The challenge-collapse algorithm consumes **exactly two** link types:

| Value | Meaning | Direction |
|---|---|---|
| `PARENT_CHILD` | Claim B is proof / a sub-claim under Claim A (structural support) | parent → child (proof flows down) |
| `CHALLENGE` | Claim B is a challenge against Claim A (adversarial) | challenger → target (attack direction) |

**Legacy types `SUPPORTS`, `EVIDENCE_FOR`, `ATTACKS` are not valid** and must not appear in new data, tooling, or docs.

> ⚠️ **Schema-drift rule:** if the `LinkType` enum in `debate.proto` ever changes, update — in the same change — this file, the parsing pipeline docs ([`pipeline-parsing.md`](pipeline-parsing.md)), and the external `paragraph-parsing` skill's Section B.

`LinkType` is marked legacy in the proto but is **still the enum that persisted rows use**. It is superseded conceptually by `RelationRole` + `basis` below, not yet in practice.

## `RelationRole` enum (new shape)

| Value | Meaning |
|---|---|
| `SUPPORTS` | The from-set props the target up. Maps from legacy `NORMAL`, `PARENT_CHILD` |
| `OPPOSES` | The from-set attacks the target. Maps from legacy `CHALLENGE` |

Genuinely closed — there is no third option — so an enum is safe here.

## `basis` (new shape, a string not an enum)

What *kind* of connection a relation is: `EVIDENTIAL`, `AUTHORITY`, `DEFINITIONAL`, `NORMATIVE`.

Deliberately a **string**. Each name maps to exactly one of four defeat behaviours (collapse / unsupport / fork / don't-propagate), so adding a basis should be a config edit rather than a proto migration plus a client regeneration. Unknown names are rejected at write time.

## `MoveType` enum

The vocabulary of the append-only log. See [`data-model.md`](data-model.md) for the table and [`backend.md`](backend.md) for which of these are actually written.

`ASSERT` · `OPPOSE` · `CONCEDE` · `ACCEPT` · `REST` · `FLAG_UNKNOWN` · `REPLACE` · `DISOWN` · `REOPEN` · `TIMEOUT` · `RETRACT` · `REDACT` · `CONTRADICT`

Two are worth distinguishing because they read differently to a user:

| Value | Means |
|---|---|
| `CONCEDE` | "You defeated this" — the author gives the claim up |
| `RETRACT` | "I am taking this back" — the author withdraws it |

`REDACT` is administrative removal (blanks text, keeps the node). No action maps to it yet.

## `TargetType` enum

`TARGET_CLAIM` · `TARGET_RELATION`

Every move written today uses `TARGET_CLAIM`. Nothing yet writes `TARGET_RELATION`, which is why `UNSUPPORTED` cannot be produced — see [`backend.md`](backend.md).

## `Status` enum (computed, never persisted)

Eleven values: `STANDING` · `ACCEPTED` · `OPEN` · `UNSUPPORTED` · `COLLAPSED` · `CONTESTED` · `UNRESOLVED` · `ABANDONED` · `SUPERSEDED` · `CIRCULAR` · `MISDIRECTED`.

Carried in the response-only `ComputedClaimState` / `ComputedRelationState` messages, deliberately kept as separate types so a status field cannot creep back onto `Claim` or `Relationship`. Behavioural meaning belongs in [`../10-concepts.md`](../10-concepts.md); which values the engine can currently produce is in [`backend.md`](backend.md).

The legacy `ClaimStatus` (`UNDETERMINED` / `TRUE_CLAIM` / `FALSE_CLAIM`) is still what current code stores and renders.

## Sequence numbers live only on `Move`

`Claim`, `Relationship` and `Debate` deliberately carry no `seq`, for two reasons:

1. **It cannot be populated.** A move is written *after* the thing it describes, so at claim-write time no seq exists. Storing one would mean write-claim, write-move, then update-claim — a second write to a row meant to be immutable.
2. **It would be a second copy.** The `ASSERT` already records when a claim entered, and `MAX(seq)` already gives a debate's head. A duplicate can only drift.

`DebateState.at_seq` / `head_seq` are the exception, and are computed per response rather than stored.

## Field-numbering discipline

Claims are persisted as **serialized blobs** (`STATEMENTS.STATEMENT_DATA`), so a field number is part of the storage format. Never renumber or retype an existing field, and never reuse a gap — `Claim` skips 4–7, which were used historically. Additive changes only.

## Message envelopes

⚠️ TODO (fill from `debate.proto` and related .proto files):
- Top-level client message type(s) and their oneof/variant structure
- Server response types, including the UI layout messages consumed by the Virtual Renderer

## Regenerating stubs

From the **repo root**: `npx buf generate`.

Full instructions, prerequisites and the reason bare `protoc` is not a substitute are in [`protos/README.md`](../../protos/README.md) — not duplicated here.
