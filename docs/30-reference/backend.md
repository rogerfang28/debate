---
layer: 3
title: Backend Reference — C++ Server & Challenge-Collapse
status: partial
last-updated: 2026-08-05
audience: developers (implementation)
source-of-truth-for: [C++ server, /clientmessage endpoint, challenge-collapse algorithm]
---

# Backend Reference — C++ Server

> **Status: partial.** This file is scaffolded from high-level knowledge; agents working in the backend should fill in the ⚠️ TODO sections from the actual source and update `last-updated`.

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

## Challenge-collapse algorithm

The core of the platform. Behavioral spec is in [`../10-concepts.md`](../10-concepts.md); this section documents the *implementation*.

Known behavior it must implement:

- **Link types consumed:** exactly `PARENT_CHILD` (support, proof flows parent → child) and `CHALLENGE` (attack, challenger → target), matching the `LinkType` enum in `debate.proto`. Legacy types (`SUPPORTS`, `EVIDENCE_FOR`, `ATTACKS`) are **not** consumed.
- **Recursive cascade on concession:** if a conceded claim was evidence in another challenge, the parent challenge fails, propagating up the chain until top-level resolution.
- **Aggregation shapes:** independent supports (any one suffices) vs. required chains (all links necessary) must collapse differently — see design case 11.

⚠️ TODO (fill from source):
- Entry point file(s) and function names for the collapse routine
- Graph traversal order and cycle handling
- How challenge states (Being Constructed / Open / Resolved) are represented and transitioned
- Where the aggregation shape (independent vs. chain) is stored per claim
- Concurrency model (threading, locking around SQLite)

## Persistence

Backed by SQLite (`debates.sqlite3`, `users.sqlite3`) — schema documented in [`data-model.md`](data-model.md).

## Build & run

⚠️ TODO: CMake targets, dependency setup, local run instructions, ports.

## Invariants (keep this list current)

- Only `PARENT_CHILD` and `CHALLENGE` link types exist in persisted data.
- A challenged user cannot respond while a challenge is **Being Constructed**.
- Concession cascades are the only automatic state propagation; abandonment must never auto-resolve to concession (design case 8).
