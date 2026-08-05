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

### Unreachable UI

Several flows exist and work server-side but have no route to them: the view that renders is `StepView`, and it has no navigation to `FullDebateView` or `SingleStatementView`.

- **Challenging a claim** — the whole challenge panel
- **Editing / modifying a claim** — `modifyClaimButton` renders only in the other views
- **Selecting a link to challenge** — the selection state is fully built and rendered, but nothing consumes it

This is the biggest single blocker. It gates `OPPOSE` reaching the log through real use, and it gates the relation-attack work that `UNSUPPORTED` depends on.

*(Two things once listed here as defects are not. Deleting a claim leaves the row and cuts its links — a deliberate choice. The Save/Cancel edit flow is a staged two-level edit in which the outer Submit is the commit point. See [`30-reference/data-model.md`](30-reference/data-model.md) and [`30-reference/backend.md`](30-reference/backend.md).)*

### Shipped

*(nothing recorded yet — move items here with a date and a link to their reference doc)*

## Design-case coverage

Per-case implementation status lives in [`11-design-cases.md`](11-design-cases.md) (table at the bottom). Treat that table as the source of truth for mechanic coverage; this page tracks feature-level roadmap only.
