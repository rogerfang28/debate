---
layer: 3
title: Protocol Reference — Protocol Buffers Contract
status: partial
last-updated: 2026-08-05
audience: developers (implementation)
source-of-truth-for: [debate.proto, LinkType enum, client/server message envelopes]
---

# Protocol Reference — Protocol Buffers

> **Status: partial.** The `LinkType` contract below is authoritative; fill remaining ⚠️ TODOs from `debate.proto`.

All communication between frontend and backend — including UI layouts — is Protocol Buffers, carried over a single `POST /clientmessage` endpoint.

## `LinkType` enum (canonical — do not drift)

The challenge-collapse algorithm consumes **exactly two** link types:

| Value | Meaning | Direction |
|---|---|---|
| `PARENT_CHILD` | Claim B is proof / a sub-claim under Claim A (structural support) | parent → child (proof flows down) |
| `CHALLENGE` | Claim B is a challenge against Claim A (adversarial) | challenger → target (attack direction) |

**Legacy types `SUPPORTS`, `EVIDENCE_FOR`, `ATTACKS` are not valid** and must not appear in new data, tooling, or docs.

> ⚠️ **Schema-drift rule:** if the `LinkType` enum in `debate.proto` ever changes, update — in the same change — this file, the parsing pipeline docs ([`pipeline-parsing.md`](pipeline-parsing.md)), and the external `paragraph-parsing` skill's Section B.

## Message envelopes

⚠️ TODO (fill from `debate.proto` and related .proto files):
- Top-level client message type(s) and their oneof/variant structure
- Server response types, including the UI layout messages consumed by the Virtual Renderer
- Versioning strategy (field numbering discipline, reserved fields)
- Where generated code lives for C++ and TypeScript, and how it's regenerated

## Regenerating stubs

⚠️ TODO: protoc invocation / build step for both languages.
