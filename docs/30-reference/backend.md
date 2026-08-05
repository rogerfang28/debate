---
layer: 3
title: Backend Reference — C++ Server & Challenge-Collapse
status: partial
last-updated: 2026-08-05
audience: developers (implementation)
source-of-truth-for: [C++ server, /clientmessage endpoint, challenge-collapse algorithm, MoveLogger, StatusEngine, build & run]
---

# Backend Reference — C++ Server

> **Status: partial.** The move log and status engine sections below are documented from source. The original challenge-collapse implementation is still ⚠️ TODO.

## Overview

- Language: **C++** (with C-level dependencies: sqlite3, protobuf)
- HTTP server: **httplib**
- API surface: a **single endpoint** — `POST /clientmessage` — carrying protobuf envelopes for every operation
- Build system: **CMake**

## Endpoint contract

`POST /clientmessage`

- Request body: protobuf-encoded client message (see [`protocol.md`](protocol.md))
- Response body: protobuf-encoded server response / UI layout
- ⚠️ TODO: document the message envelope types, dispatch mechanism, auth/session handling, and error responses from source.

Note: the response carries a full `user::User`, and `buildResponseMessage` clears `password_hash` from it before returning. That is defence in depth rather than a fix for a live leak — the message is server-internal today, since only the `ui::Page` is serialized to the client.

## Challenge-collapse algorithm (the current engine)

The core of the platform. Behavioral spec is in [`../10-concepts.md`](../10-concepts.md); this section documents the *implementation*.

Known behavior it must implement:

- **Link types consumed:** exactly `PARENT_CHILD` (support, proof flows parent → child) and `CHALLENGE` (attack, challenger → target), matching the `LinkType` enum in `debate.proto`. Legacy types (`SUPPORTS`, `EVIDENCE_FOR`, `ATTACKS`) are **not** consumed.
- **Recursive cascade on concession:** if a conceded claim was evidence in another challenge, the parent challenge fails, propagating up the chain until top-level resolution.
- **Aggregation shapes:** independent supports (any one suffices) vs. required chains (all links necessary) must collapse differently — see design case 11.

Status here is **stored** (`Claim.status` plus a per-viewer `user_statuses` map) and mutated in place. Two engines exist in `DebateWrapper` — `UpdateStatusOfAllClaimsInDebate`, and `PropagateClaimStatuses` with `ComputeStatusForUser` — and they are only loosely consistent with each other.

⚠️ TODO (fill from source):
- Entry point file(s) and function names for the collapse routine
- Graph traversal order and cycle handling
- How challenge states (Being Constructed / Open / Resolved) are represented and transitioned
- Where the aggregation shape (independent vs. chain) is stored per claim
- Concurrency model (threading, locking around SQLite)

## Move log — `MoveLogger`

`backend/src/moveLog/MoveLogger.{h,cc}`. The **only** place that decides what counts as a move. No handler and no part of `DebateWrapper` knows the log exists; deleting these two files and two lines in `DebateModerator` removes the feature entirely.

`DebateModerator::handleRequest` wraps dispatch with two calls:

```
pre = MoveLogger::capture(event, ...)     // before
handleDebateEvent(...)                    // unchanged
MoveLogger::logForEvent(event, pre, ...)  // after
```

Split around dispatch because an event describes **intent** while the log records **outcome**, and neither is readable at one moment: deletions must be read before (afterwards the row is gone and its debate unknowable), creations after (beforehand the id does not exist).

**Known fragility:** handlers do not report what they created — `AddClaimUnderClaim` returns the new id and its caller discards it — so `MoveLogger` re-derives it, finding new child claims by diffing the parent's children and new debates by diffing the user's debate list. A handler that changes what it sets can make this log a wrong id *silently* rather than failing to compile. This is not hypothetical: an early version read the new debate from the user's engagement, but `AddDebate` never moves the user into the debate it creates, so the root-claim `ASSERT` was dropped with a clean build and green tests.

### What is logged

| Action | Move | Target |
|---|---|---|
| create debate | `ASSERT` | the root claim |
| add child claim | `ASSERT` ×2 | the claim, then the relation |
| submit challenge | `ASSERT` then `OPPOSE` | the new claim, then **the challenged claim** |
| concede | `CONCEDE` | the claim given up |
| delete claim / link / challenge | `RETRACT` | the claim or relation |

A `CHALLENGE` link logs `OPPOSE`, not `ASSERT`: creating that link *is* the act of opposing, so it targets the claim under attack with the relation id in the payload.

Deletions are hooked in the **handlers**, not in `deleteClaim`/`deleteLinkById`, because those are also used to tear down a whole debate — hooking them would emit a `RETRACT` per claim during `DELETE_DEBATE`, which is not logged.

### What is deliberately not logged

- `DELETE_DEBATE`, `CLEAR_DEBATES` — they destroy the log too, so there is nothing to reconstruct into
- `JOIN_DEBATE` / `LEAVE_DEBATE` — membership, not argument moves. **This is a known gap:** `REST` and `TIMEOUT` need to know who was present to determine who went quiet, and nothing records that
- `CANCEL_MODIFICATION_OF_CLAIM`
- `REPLACE` on edit submit — see the defect below

### Editing is a two-level, staged operation

Worth understanding before adding `REPLACE` logging, because there are **two** submits and they do different things:

| Control | Event | Effect |
|---|---|---|
| Inner **Save** on the edit box | `SUBMIT_EDIT_CLAIM` | writes the new text; closes only the edit box. The modification session stays open |
| Outer **Submit** on the modify panel | `SUBMIT_MODIFICATION_OF_CLAIM` | clears `modifying_current_claim` — session closes, the edit stands |
| Outer **Cancel** | `CANCEL_MODIFICATION_OF_CLAIM` | restores the snapshot taken at Start, discarding the session's edits |

So `START_MODIFICATION_OF_CLAIM` snapshots into `history`, inner Save writes text, and the **outer Submit/Cancel is the real commit point**. Cancel discarding the session is the intended meaning, not data loss.

That makes the outer Submit the natural place to log `REPLACE` when the time comes — logging on the inner Save would record a change the outer Cancel can still discard.

One wrinkle if you touch this: inner Save persists straight to the database rather than staging in memory, so a concurrent reader could briefly observe an edit that is later discarded. Invisible single-user. Also `editClaimText` writes both the blob and the `TEXT` column while the restore path (`updateClaimInDB`) writes only the blob, so a cancelled edit leaves `TEXT` holding the discarded value; the UI reads the blob, so it is not user-visible today.

**Currently unreachable:** `modifyClaimButton` renders only in `FullDebateView` and `SingleStatementView`, and the view that actually renders is `StepView`, which has no navigation to either. The whole edit/modify flow — like the challenge flow — cannot be triggered by a user today.

## Status engine — `StatusEngine`

`backend/src/statusEngine/StatusEngine.{h,cc}`. A **pure** function:

```cpp
Result computeStatuses(claims, relations, moves)
```

No database, no clock, no globals; same inputs always give the same outputs. It never reads `Claim.status` or `user_statuses` — consulting the old model would defeat the point.

**Currently produces only `STANDING`, `OPEN`, `COLLAPSED`.** Behavioural meaning of each status is in [`../10-concepts.md`](../10-concepts.md).

Two things easy to get backwards, both pinned by tests:

- **Cascade runs against the stored link direction.** A `PARENT_CHILD` link is stored parent → child, but the child is *evidence for* the parent, so collapse travels child → parent. Conceding a conclusion must **not** destroy its evidence, which may support something else.
- **`CHALLENGE` links never carry collapse.** If they did, withdrawing your own challenge would destroy the claim you were attacking.

`OPPOSE` alone never defeats anything — it makes its target `OPEN`. An attack has no mechanical success condition; it bites only via `CONCEDE` or `TIMEOUT`. An attack also only counts while its own claim stands, so a withdrawn challenge lets its target recover.

### Why the other eight statuses cannot be produced

| Reason | Statuses |
|---|---|
| The move is never written — no such action exists | `ACCEPTED`, `CONTESTED`, `UNRESOLVED`, `ABANDONED`, `MISDIRECTED`, `SUPERSEDED` |
| The **distinction** was never recorded | `UNSUPPORTED` |
| Implementable now, deliberately out of scope | `CIRCULAR` |

`UNSUPPORTED` is the important one: it needs an `OPPOSE` against a *relation* (the golden rule — your evidence is real but doesn't prove your point), but every move written uses `TARGET_CLAIM`, and `basis` is never stored. That is not an engine gap; the fact has never been in the log.

### Shadow mode

`BuildCollection` calls `computeStatuses` alongside the stored status, logs disagreements, and **discards the result** — nothing renders or decides from it. Debates with no moves are skipped, since anything created before the log has nothing to replay.

Disagreements are expected and are the point: the stored value is per-viewer while the computed one is global, so some divergence is structural. The shape of the disagreement is what is worth reading, not its volume.

## Persistence

Backed by SQLite (`debates.sqlite3`, `users.sqlite3`) — schema documented in [`data-model.md`](data-model.md).

## Build & run

```bash
cd backend
cmake -S . -B build
cmake --build build -j
```

Requires MinGW-w64 (GCC 15+), Ninja, and vcpkg at `third_party/vcpkg` with `x64-mingw-static` packages (protobuf, sqlite3, abseil, openssl, utf8-range). New `.cc` files must be added to `SRC_FILES` in `backend/CMakeLists.txt`; generated protobuf sources are globbed automatically.

Run: `backend/build/bin/debate_server.exe`. Environment: `DEBATE_SERVER_PORT` (default 8080), `GOOGLE_CLIENT_ID` (omitted → no Google sign-in button), `DB_PATH`.

Tests live in `test/` with their own CMake project — see `test/README.md`.

## Invariants (keep this list current)

- Only `PARENT_CHILD` and `CHALLENGE` link types exist in persisted data.
- A challenged user cannot respond while a challenge is **Being Constructed**.
- Concession cascades are the only automatic state propagation; abandonment must never auto-resolve to concession (design case 8).
- **`MoveLogger` is the only place that decides what a move is.** No handler writes to the log.
- **A failed log write never breaks the operation that triggered it.** Verified by a test that drops the `MOVES` table and asserts claims can still be created.
- **`computeStatuses` is pure and never reads stored status.**
- **Nothing reads the move log to render or decide.** It is a parallel record until the engine takes over.
