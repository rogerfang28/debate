---
layer: 3
title: Frontend Reference — Server-Driven UI
status: partial
last-updated: 2026-08-07
audience: developers (implementation)
source-of-truth-for: [React/TS frontend, Virtual Renderer, polling loop, graph layout, junction nodes, form-field collection]
---

# Frontend Reference — React/TS Server-Driven UI

> **Status: partial.** Scaffolded from high-level knowledge; fill ⚠️ TODO sections from source when working here.

## Overview

- **React + TypeScript (ESM)** — the revamped frontend. A **legacy JavaScript frontend** still exists in the repo (~9% of the codebase); prefer the TS one for new work and note here which paths are legacy.
- **Server-driven UI:** the server sends UI *layouts* as protobuf; the client renders what it's told. The client holds minimal presentation logic of its own.
- **Virtual Renderer** ("middle-end"): an in-process translator that converts backend/protobuf layout structures into what React renders.
- **Real-time:** HTTP POST polling against `/clientmessage`.

## Rendering pipeline

```
protobuf layout (from server)
   → Virtual Renderer (translate to renderable structure)
   → React components (render)
```

### How a form field reaches the backend

The contract between server and client is **just the component id string** — there is no shared schema for form fields:

1. Backend emits a component with an id (`ComponentGenerator::create*`)
2. The component renders as a real DOM element carrying that id
3. On submit, `getEntirePage.ts` scans the DOM for `input[id]`, `textarea[id]`, `select[id]` and collects `{id: value}` — generically, naming no field
4. `postClientMessageToCPP.ts` turns every entry into `ComponentData{id, value}`
5. `DebatePageEventParser` matches on the id and sets the event field

Steps 3–4 are field-agnostic, so **adding a form field is a backend-only change**: emit it with an id, add a matching branch in the parser. Nothing validates that the two ids agree, so a typo in either place yields an empty value rather than an error.

### Graph rendering — `GraphComponent.tsx`

The backend sends `nodes` and `edges` with **no coordinates**. The frontend owns layout entirely: it builds a children map, picks a root, BFS's levels, computes subtree widths, and assigns every `x`/`y`.

**Edges run in relation orientation:** `SUPPORTS` edges point *supporter → supported* (child → parent), up the tree. The layout inverts them when building `childrenMap` so it can still draw top-down. Root detection needs no special case and works for the right reason — the root is the claim that supports nothing, which is exactly the node that is never a source.

Why the orientation matters: several claims supporting one conclusion must **share a target** so they converge on a single junction. In the old parent → child orientation sibling edges shared a *source* and fanned out, so there was nowhere to put one.

Edge-type checks are **allow-lists** (`isSupportEdge` / `isChallengeEdge`), not "not a challenge". A stale cached page or saved layout JSON can still carry the old `PARENT_CHILD` name, and treating an unrecognised type as support would silently mis-draw the tree.

### Junction nodes

Every relation draws as its own small node on the edge: sources point **at** it, and it alone points at the target. It carries the relation's basis, and it is what an attack on the reasoning will target when R3 arrives.

- **Grouped by `relationId`.** A relation with several from-claims arrives as several edges sharing that id and draws **one** junction. This is what makes the two readings distinguishable: N edges into one target can mean "these N together support it" (one relation) or "each independently supports it" (N relations). Edges with `relationId` 0 stand alone rather than merging, since lumping unattributed edges would invent a relation nobody asserted.
- **Positioned geometrically**, midway between the sources' centre of mass and the target — *not* inserted into the tree layout as real levels. Making them layout nodes would double every tree's depth and force row spacing, subtree widths and root detection to be reworked, for a node with no children of its own. With one source the position is exactly the old edge midpoint.
- **Only the outgoing segment carries an arrowhead.** The relation makes one assertion about its target, however many claims feed it.
- **The label falls back to the role** (`SUPPORTS`/`OPPOSES`) when no basis was chosen, so a junction is never blank and never claims a basis nobody selected. The pre-dropdown placeholder string is treated as absent — see `RelationBuilder` in [`backend.md`](backend.md), which deliberately does not launder it into a real basis.

⚠️ TODO (fill from source):
- Directory layout: where the TS frontend lives vs. legacy JS
- The Virtual Renderer's module/entry point and its layout node types
- Polling loop location, interval, and backoff behavior
- Component inventory for debate views (claim node, challenge panel, concede/counter controls)

## Conventions

- All new frontend code in **TypeScript**, ESM modules.
- No client-side business logic for debate resolution — the backend is authoritative; the client renders state.

## Legacy JS frontend

⚠️ TODO: list legacy paths, what still depends on them, and the migration plan/status.
