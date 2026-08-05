---
layer: 3
title: Frontend Reference — Server-Driven UI
status: partial
last-updated: 2026-08-05
audience: developers (implementation)
source-of-truth-for: [React/TS frontend, Virtual Renderer, polling loop]
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

⚠️ TODO (fill from source):
- Directory layout: where the TS frontend lives vs. legacy JS
- The Virtual Renderer's module/entry point and its layout node types
- Polling loop location, interval, and backoff behavior
- How user actions are serialized into client messages
- Component inventory for debate views (claim node, challenge panel, concede/counter controls)

## Conventions

- All new frontend code in **TypeScript**, ESM modules.
- No client-side business logic for debate resolution — the backend is authoritative; the client renders state.

## Legacy JS frontend

⚠️ TODO: list legacy paths, what still depends on them, and the migration plan/status.
