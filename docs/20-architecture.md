---
layer: 2
title: Architecture
status: current
last-updated: 2026-08-05
audience: developers
---

# Architecture

The map of the system: what the parts are and how they talk. Deep detail on each part lives in [`30-reference/`](30-reference/) — this page should stay short enough to read in five minutes.

## Component diagram

```
┌────────────────────────────┐
│  Frontend                  │  React + TypeScript (ESM)
│  server-driven UI          │  layouts sent as protobuf
└─────────────┬──────────────┘
              │  HTTP POST polling (real-time)
              │  Protocol Buffers payloads
┌─────────────▼──────────────┐
│  Virtual Renderer          │  "middle-end": in-process translator
└─────────────┬──────────────┘
┌─────────────▼──────────────┐
│  Backend                   │  C++ + httplib
│  single endpoint:          │  POST /clientmessage
│  challenge-collapse algo   │
└─────────────┬──────────────┘
┌─────────────▼──────────────┐
│  SQLite                    │  debates.sqlite3  +  users.sqlite3
└────────────────────────────┘
```

## The parts

| Component | Tech | Role | Deep dive |
|---|---|---|---|
| Frontend | React + TypeScript (ESM) | Server-driven UI; renders layouts sent from the server as protobuf | [`30-reference/frontend.md`](30-reference/frontend.md) |
| Virtual Renderer | in-process | "Middle-end" translator between backend state and UI layouts | [`30-reference/frontend.md`](30-reference/frontend.md) |
| Backend | C++ with httplib | All logic, including the challenge-collapse algorithm; exposes a **single POST endpoint** `/clientmessage` | [`30-reference/backend.md`](30-reference/backend.md) |
| Protocol | Protocol Buffers | Contract for **all** communication (including UI layouts) | [`30-reference/protocol.md`](30-reference/protocol.md) |
| Database | SQLite | Two files: `debates.sqlite3` and `users.sqlite3` | [`30-reference/data-model.md`](30-reference/data-model.md) |
| Real-time | HTTP POST polling | Client polls the server for updates | [`30-reference/frontend.md`](30-reference/frontend.md) |
| Parsing pipeline | Python/FastAPI + LLM (separate tooling) | Extracts claim trees from articles for the Debate Tool pipeline | [`30-reference/pipeline-parsing.md`](30-reference/pipeline-parsing.md) |

## Language breakdown (repo)

C++ 52% (backend) · C 30% (sqlite3, protobuf low-level) · JavaScript 9% (legacy frontend) · TypeScript 5% (revamped frontend) · CSS 2% · CMake 1% (build).

## Key architectural decisions

Record significant decisions here as short entries (or split into an `adr/` folder once there are more than ~10).

| # | Decision | Rationale | Status |
|---|---|---|---|
| 1 | Server-driven UI (layouts as protobuf) | Keeps debate logic and presentation authority on the server; thin client | active |
| 2 | Single POST endpoint `/clientmessage` | One uniform protobuf envelope instead of a REST surface | active |
| 3 | Protobuf for all communication | Typed contract shared by C++ backend and TS frontend | active |
| 4 | HTTP POST polling for real-time | Simplest thing that works; revisit (SSE/WebSocket) if latency matters | active |
| 5 | Two SQLite files (debates / users) | Separates identity data from debate graph data | active |

## Data flow (one round trip)

1. User acts in the UI → frontend serializes a protobuf message → `POST /clientmessage`.
2. Backend decodes, mutates debate graph state (runs challenge-collapse if needed), persists to SQLite.
3. Backend responds (and subsequent polls return) protobuf-encoded state / UI layout.
4. Virtual Renderer translates the layout; React renders it.
