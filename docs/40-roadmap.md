---
layer: 4
title: Status & Roadmap
status: current
last-updated: 2026-08-05
audience: everyone
---

# Status & Roadmap

## Current status

- Solo project by Roger Fang
- ~320 commits, 17 branches; no releases published yet
- Latest known commit: Mar 31, 2026
- Live demo (WIP, may be down): https://debate-spxg.onrender.com/

## Roadmap

Keep this list ordered by priority. When an item ships, move it to **Shipped** with the date and link the docs that describe it.

### In progress — moving status from stored to computed

The large one. Status today is written into each claim and mutated in place; the goal is to compute it from an append-only log instead. Being done in parallel so nothing breaks: the log is written and the engine runs, but nothing reads either yet.

- [x] Append-only `MOVES` table, seq per debate — [`30-reference/data-model.md`](30-reference/data-model.md)
- [x] `MoveLogger` at the dispatch point — [`30-reference/backend.md`](30-reference/backend.md)
- [x] `debate.proto` extended for hyperedges, moves and the 11 statuses — [`30-reference/protocol.md`](30-reference/protocol.md)
- [x] `computeStatuses` as a pure function (`STANDING`, `OPEN`, `COLLAPSED`)
- [x] Shadow mode — computed vs stored, logged and discarded
- [ ] **Attacking a relation.** Every move written targets a claim, so `UNSUPPORTED` — the distinction the whole design rests on — has never entered the log. Needs a `TO_KIND` on `LINKS`, `addLink` to stop assuming both ends are claims, and relations to be clickable in the UI
- [ ] The six statuses blocked on actions that do not exist: `ACCEPT`, `REST`, `FLAG_UNKNOWN`, `DISOWN`, `TIMEOUT`, and `REPLACE`
- [ ] Participation history — join/leave are not recorded, so `REST` and `TIMEOUT` cannot tell who was present
- [ ] Retire the legacy engine once the computed one is trusted

### Planned

- [ ] Refactor
- [ ] History / version navigation
- [ ] Visual credibility indicators
- [ ] Full multiplayer
- [ ] AI summaries
- [ ] More debate constraints

### Known defects

- **"Save" on a claim edit can be silently undone by "Cancel".** Editing is only reachable inside a modification session, and submitting persists immediately without closing that session — so the still-visible Cancel restores the pre-edit snapshot and discards the saved change. Details in [`30-reference/backend.md`](30-reference/backend.md). Resolves when edits become `REPLACE` + a new immutable claim.
- **Deleting a claim orphans it rather than removing it.** The row stays in `STATEMENTS`; only its links are deleted. Any query not joining through `LINKS` still counts it — see [`30-reference/data-model.md`](30-reference/data-model.md).

### Shipped

*(nothing recorded yet — move items here with a date and a link to their reference doc)*

## Design-case coverage

Per-case implementation status lives in [`11-design-cases.md`](11-design-cases.md) (table at the bottom). Treat that table as the source of truth for mechanic coverage; this page tracks feature-level roadmap only.
