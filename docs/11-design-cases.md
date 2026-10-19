---
layer: 1
title: Design Cases — Scenario Requirements
status: current
last-updated: 2026-08-05
audience: designers, contributors, testers
---

# Design Cases — The 11 Scenarios the Tool Must Handle

These are the acceptance criteria for the debate mechanics. Each case has a **Situation**, what the tool **Must support**, and a **Passes if** test. Treat these as the spec: any change to the challenge/collapse mechanism should be checked against every case here.

> Implementation status per case is tracked in the table at the bottom. Update it whenever a case's status changes.

## 1. Claim with no true/false answer

- **Situation:** A claim can't be proven true or false — a value/policy claim ("X should be banned") or a broad claim that's partly true and partly false.
- **Must support:** Non-binary resolution. Decomposing into narrower, individually debatable points. An aggregate output reflecting the balance of those points without declaring the top claim "true" or "false."
- **Passes if:** Defeating one sub-point changes the picture without automatically resolving the whole claim, and the final output is something a reader can *weigh*, not a verdict the tool asserts.

## 2. The evidence is true but doesn't establish the claim

- **Situation:** Support is factually accurate (real data, real study) but doesn't establish the claim — e.g. a real correlation presented as proof of causation.
- **Must support:** Accepting support as true while rejecting its *bearing* on the claim. Distinguishing "the reasoning connecting support to claim failed" from "the support is false" — and both from "the claim is false."
- **Passes if:** A user can concede the evidence is real and still defeat its *use*, and the parent claim's resulting state is visibly different from a direct refutation.

## 3. Two parties use a key term differently

- **Situation:** Same word, different meanings — the parties appear to clash but never address the same proposition.
- **Must support:** Detecting/representing that a disagreement rests on divergent meanings. Gating the substantive argument on settling the meaning first.
- **Passes if:** The tool surfaces "these two are not addressing the same proposition" and holds the clash open until the definitional question resolves — ideally caught by a mechanism, not by a human noticing.

## 4. The source's credibility is attacked, not the claim

- **Situation:** "That study is industry-funded" — the data itself is undisputed.
- **Must support:** Challenging credibility/provenance of support separately from the content it backs.
- **Passes if:** A successful source attack degrades/removes that line of support without by itself making the claim false. Truth status and credibility status move independently.

## 5. The attack targets a person or source, not a claim

- **Situation:** Attacks on *who* is speaking — from evidence-backed meta-arguments ("this article is AI-written, here's a detector score") down to bare insults. Targets the author or a participant; attaches to no specific claim.
- **Must support:** A place where standing/credibility attacks are real, resolvable debates but structurally separate from claim truth. No pre-sorting by quality — strong meta-arguments and lazy insults enter the same way and are sorted by whether they *survive challenge*.
- **Passes if:** Such an attack can be fully debated, can flag the argument's or source's credibility if it succeeds, yet cannot by itself collapse any individual claim.

> **Note — cases 4 and 5 are one requirement at different scopes.** Source credibility is attackable independently of claim truth, whether the source is a single study (4), the author (5), or a participant (5). A strong solution handles all with **one mechanism**, varying only what the credibility attaches to. If a design treats ad hominem, author-attacks, and funding-attacks as three separate features, collapse them into one.

## 6. Nobody actually knows yet

- **Situation:** Evidence needed to settle a point is sparse or nonexistent. The honest answer is "unknown."
- **Must support:** An outcome for open/insufficient-evidence questions distinct from "defeated," which a party can challenge by producing the evidence if it exists.
- **Passes if:** An open question is marked open, does not propagate as a defeat, and does not drag down claims depending on it. *A binary true/false structure fails this automatically.*

## 7. A challenge aimed at a claim that was never made

- **Situation:** Strawman — the attack points at something not explicitly (or only implicitly) in the argument.
- **Must support:** Validating that a challenge's target was actually asserted or genuinely implied before it lands. Handling "you're attacking something I never said" — and the counter, "you did imply it, here."
- **Passes if:** A misdirected challenge is neither silently dropped nor allowed to fabricate a target — it's surfaced as misdirected. If genuinely implied, there's a path to make the implied claim explicit so the challenge can land on it.

## 8. Nobody concedes — the thread just ends

- **Situation:** The real-world default: no concession, no refutation; people just leave. Silence is ambiguous.
- **Must support:** Distinguishing abandonment from concession.
- **Passes if:** An unanswered branch resolves to something explicitly meaning "abandoned," never silently to "conceded," and the structure records **who** went silent (an attacker leaving and a defender leaving mean different things).

## 9. Agreement is not support

- **Situation:** On a popular post, casual readers mistake volume of agreement for strength of support.
- **Must support:** Keeping social/agreement signals distinct from argumentative support.
- **Passes if:** A claim with heavy agreement but no surviving supporting arguments is *visibly unsupported*; popularity and support are distinguishable in the output.

## 10. Both sides have real, surviving evidence

- **Situation:** Both sides produce genuine evidence; neither refutes the other. The disagreement is over whose evidence counts.
- **Must support:** An outcome for "both positions have surviving support and there is no winner." No forced verdicts.
- **Passes if:** When two supported branches both survive scrutiny, the tool reports a genuine standoff rather than manufacturing a winner.

## 11. Independent supports vs. a required chain

- **Situation:** Some claims rest on independent arguments (any one suffices); others on a chain (every link required).
- **Must support:** Both aggregation shapes with correct collapse behavior for each — ideally without special-case flags per combination.
- **Passes if:** Defeating one of several independent supports leaves the claim standing; defeating one link of a required chain breaks it — and the user can express which shape an argument has.

---

## Implementation status

| # | Case | Status | Notes / where implemented |
|---|------|--------|---------------------------|
| 1 | Non-binary claims | 🔲 not started | |
| 2 | True evidence, failed inference | 🔲 not started | |
| 3 | Divergent definitions | 🔲 not started | |
| 4 | Source credibility attack | 🔲 not started | Shared mechanism with #5 |
| 5 | Person/source attack (no claim) | 🔲 not started | Shared mechanism with #4 |
| 6 | Unknown / open outcome | 🔲 not started | |
| 7 | Strawman validation | 🔲 not started | |
| 8 | Abandonment vs. concession | 🔲 not started | |
| 9 | Agreement ≠ support | 🔲 not started | |
| 10 | Standoff outcome | 🔲 not started | |
| 11 | Independent vs. chained supports | 🔲 not started | |

Status legend: 🔲 not started · 🟡 partial · ✅ implemented · 🧪 has tests
