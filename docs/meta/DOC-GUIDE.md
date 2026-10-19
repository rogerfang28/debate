# DOC-GUIDE — How to Maintain This Documentation

**Audience: any human or AI agent editing code or docs in this repo. Read this before modifying anything under `docs/`.** These rules are what keep the documentation layered, non-duplicated, and trustworthy as the project grows.

---

## 1. The layer model

| Layer | Files | Contains | Never contains |
|---|---|---|---|
| 0 | `00-overview.md` | Mission, problem, elevator pitch | Mechanics, code, file paths |
| 1 | `10-*.md`, `11-*.md` | Behavioral spec: how the system *behaves*, requirements | Implementation details, function names, schemas |
| 2 | `20-architecture.md` | Component map, tech choices, data flow, decisions | Function-level detail, full schemas |
| 3 | `30-reference/*.md` | Implementation truth: files, functions, schemas, invariants, build steps | Duplicated concept explanations (link to Layer 1 instead) |
| 4 | `40-roadmap.md` | Status, priorities, shipped log | Anything permanent |
| meta | `meta/*` | These rules, templates, changelog | Project content |

**The single-home rule:** every piece of information lives at exactly **one** layer. Other layers may *link* to it, never restate it. If you're about to copy a paragraph between files, replace the copy with a link.

**The right-layer test:** ask "who needs this?" A non-technical reader → Layer 0–1. A developer orienting → Layer 2. A developer implementing → Layer 3. If a sentence mixes audiences, split it across layers.

## 2. Frontmatter (required on every doc except this guide and README)

```yaml
---
layer: <0|1|2|3|4>
title: <Doc title>
status: current | partial | stale | deprecated
last-updated: YYYY-MM-DD
audience: <who this is for>
source-of-truth-for: [<topics this file owns>]   # Layer 3 only
---
```

- `status: partial` means the doc has ⚠️ TODO sections — acceptable, and better than wrong prose.
- `status: stale` means the code has moved and the doc is known-outdated. Mark stale the moment you notice; a marked-stale doc is safe, an unmarked one is a trap.
- Bump `last-updated` on **any** substantive edit.

## 3. When you change code, you change docs (the sync contract)

In the **same change/PR** as a code modification:

1. Identify the affected component and open its file in `30-reference/`. Update the sections your change touches — especially **Invariants** and any contract tables.
2. If the change alters *behavior* (not just implementation), also update the relevant Layer 1 doc (`10-concepts.md` or the status table in `11-design-cases.md`).
3. If the change adds/removes/replaces a *component* or reverses an architectural decision, update `20-architecture.md` (component table + decisions table).
4. Check the doc's **Sync obligations** section (if present) — some contracts fan out (e.g. `LinkType` changes touch `protocol.md`, `pipeline-parsing.md`, and the external `paragraph-parsing` skill).
5. Add a line to `meta/CHANGELOG.md`.
6. Bump `last-updated` and adjust `status` on every file you touched.

If you cannot update a doc properly (out of scope, out of time), you must at minimum set its `status: stale` and add a one-line note at the top saying what changed. Never leave a doc silently wrong.

## 4. Adding new documentation

- **New component or subsystem** → copy `meta/TEMPLATE-reference.md` into `30-reference/<name>.md`, fill it, then add one row to the component table in `20-architecture.md` and one line to the file map in `README.md`.
- **New behavioral requirement / hard case** → add a numbered case to `11-design-cases.md` using the Situation / Must support / Passes if format, and add a row to its status table.
- **New concept or mechanic** → new section in `10-concepts.md` (or a new `1x-` file if it's large enough to stand alone; update README's map).
- **Do not** create top-level files outside the numbering scheme. If a doc doesn't fit any layer, it probably belongs as a section of an existing file.

## 5. Writing rules

- **Filling a ⚠️ TODO:** replace it with verified content from source code, not from memory or inference. If you only *suspect* something, write "(unverified)" next to it.
- **Invariants sections** in Layer 3 docs are contracts. Only add an invariant you've confirmed the code enforces; only remove one when the code genuinely no longer enforces it (and say why in the changelog).
- Prefer tables for contracts (enums, endpoints, schemas) and short prose for explanations.
- Every doc links **down** to deeper detail and **up** to context. A reader should never dead-end.
- Keep `20-architecture.md` readable in ~5 minutes; push overflow down to Layer 3.
- Use relative links between docs so they work on GitHub and locally.

## 6. Changelog convention

Append (newest first) to `meta/CHANGELOG.md`:

```
- YYYY-MM-DD — <files touched> — <one-line summary> — <author/agent>
```

## 7. Quick checklist for agents (run before finishing any task)

- [ ] Did my code change alter any documented behavior, contract, schema, or invariant? → updated the Layer 3 doc
- [ ] Did it change *behavior* visible in Layer 1? → updated `10-concepts.md` / case status table
- [ ] Did it add/remove a component or decision? → updated `20-architecture.md` + README map
- [ ] Any doc I couldn't fully update → marked `status: stale` with a note
- [ ] Frontmatter `last-updated` bumped on every touched doc
- [ ] Line added to `meta/CHANGELOG.md`
- [ ] No information duplicated across layers (links instead)
