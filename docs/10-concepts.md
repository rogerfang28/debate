---
layer: 1
title: Core Concepts — The Challenge System
status: current
last-updated: 2026-08-05
audience: users, designers, contributors
---

# Core Concepts — The Challenge System

This is the behavioral spec of the platform: how a debate proceeds, independent of any implementation detail. For the code that implements this, see [`30-reference/backend.md`](30-reference/backend.md).

## Debates are graphs

A debate is a **recursive graph** (tree-like). Claims are nodes; challenges create **sub-debates nested inside** the graph. Two canonical link types connect claims (these are the only types the backend consumes — see [`30-reference/protocol.md`](30-reference/protocol.md)):

- **`PARENT_CHILD`** — "Claim B is proof / a sub-claim under Claim A" (structural support; proof flows parent → child)
- **`CHALLENGE`** — "Claim B is a challenge against Claim A" (adversarial; attack flows challenger → target)

## Lifecycle of a challenge

1. **Claim** — a user makes a claim on a topic.
2. **Challenge (Being Constructed)** — an opponent builds a sub-debate inside the challenge. While it's under construction, the challenged user **cannot respond yet**. This prevents piecemeal dodging: you answer the whole challenge, not fragments of it.
3. **Challenge (Open)** — the challenger marks it done. The challenged user must now respond, with exactly two options:
   - **Concede** — the claim is marked *successfully challenged*. This triggers a **recursive cascade**: if that claim was serving as evidence in another challenge, the parent challenge fails too, and so on up the chain.
   - **Counter-challenge** — challenge a claim *inside* the opponent's challenge, creating a new nested challenge.
4. **Resolution** — recursion terminates when a claim is successfully challenged or successfully defended at the top level.

## Three key capabilities

| Capability | What it does |
|---|---|
| **Force proof** | Challenging a claim forces evidence; the opponent must respond or concede. |
| **Claim reformulation** | Restate an opponent's argument in a more objective/scientific form that's easier to evaluate. |
| **Narrative breakdown** | Break complex claims into sub-statements; disproving one sub-statement breaks the larger claim. |

## Properties the system enforces

- **No dodging, ad hominem, or topic changes** — the structure simply has no move for them (attacks on people are routed to a separate credibility track; see design cases 4–5).
- **Visible accountability** — judges/readers see which claims were defended vs. conceded.
- **Accumulated knowledge** — resolved challenges build a searchable database of rebuttals and evidence.

## Aggregation shapes (independent supports vs. required chains)

A claim can rest on:

- **Independent supports** — several arguments, any one of which suffices. Defeating one leaves the claim standing.
- **A required chain** — every link necessary. Defeating any link breaks the whole.

The collapse mechanism must behave correctly for both shapes, and users must be able to express which shape an argument has (design case 11).

## Outcomes are not binary

Resolution is richer than true/false. The design must distinguish (see [`11-design-cases.md`](11-design-cases.md) for full requirements):

- **Defended** vs. **Conceded/Defeated**
- **Open / unknown** — evidence doesn't exist yet; not a defeat, doesn't cascade (case 6)
- **Abandoned** — a party went silent; explicitly *not* the same as conceded, and it matters *who* left (case 8)
- **Standoff** — both sides have surviving support; no manufactured winner (case 10)
