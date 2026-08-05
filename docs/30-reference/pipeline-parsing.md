---
layer: 3
title: Pipeline Reference — Article → Claim-Tree Extraction
status: current
last-updated: 2026-08-05
audience: developers (implementation)
source-of-truth-for: [paragraph-parsing pipeline, claim-tree-viewer app]
related-external: [paragraph-parsing skill (SKILL.md), mermaid-tree-visualization skill]
---

# Pipeline Reference — Article → Claim-Tree Extraction

Tooling that converts article text into structured debate-tree data compatible with the challenge-collapse mechanism. The operational source of truth is the external **`paragraph-parsing` skill** (`SKILL.md`); this page summarizes it and records where it plugs into the Debate Tool.

## Pipeline

```
Article text → Paragraph segmentation → Atomic claims → Link identification → JSON output
```

## Atomicity rules (segmentation)

- **One proposition per claim** — split compound "and" sentences into separate assertions
- **Attributed** — keep author/publication/date with every claim
- **Verifiable** — claims must be checkable (not bare opinions)
- **Complete** — preserve context; never take claims out of meaning
- **Neutral framing** — state claims in the article's voice, with hedging ("some say") removed

Watch for over-segmenting (one phenomenon described two ways ≠ two claims) and under-segmenting (two independent assertions joined by "and" = two claims).

## Link identification

Only the two canonical types from `debate.proto` (see [`protocol.md`](protocol.md)):

- `PARENT_CHILD` — support/proof (parent → child)
- `CHALLENGE` — adversarial (challenger → target)

Edges: `{ "type": "PARENT_CHILD"|"CHALLENGE", "source": "claim_id", "target": "claim_id", "label": "optional" }` — exact strings, no variants. Don't force artificial links; some articles (e.g. myth-busting lists) legitimately have few.

## Output schemas

Two distinct formats exist — **do not conflate them**:

1. **Nodes/edges schema** (backend-compatible): `{title, source, url, nodes[], edges[]}` where nodes carry `id`, `text`, `source_metadata`, `role` (CLAIM / COUNTER_CLAIM / EVIDENCE).
2. **Claim-tree hierarchy schema** (Claim Tree Viewer app): `{title, source, url, core, claims[]}` with per-claim `supporting {evidence, facts[]}` and optional `challenging {evidence, counter-evidence, facts[]}`. `challenging` is omitted when the article doesn't address pushback — no phantom challenges.

## Claim Tree Viewer app

Standalone web app at `workspace/paragraph-parsing/claim-tree-viewer/`:

- **Backend:** FastAPI (`api_server.py`) — `POST /api/extract` accepts `{"url"}` or `{"text"}`; fetches/strips HTML, truncates input to 15,000 chars, strips ```` ```json ```` fences before parsing
- **Prompt:** `extract_prompt.txt`, loaded at runtime (single source of truth; restart backend after editing)
- **Frontend:** React + Vite + Tailwind, renders Mermaid.js trees (`buildMermaidTree.ts`), dark theme
- **LLM:** local Ollama endpoint, Qwen model, max 4000 tokens
- Mermaid color scheme: core blue `#2563eb`, claim slate `#475569`, support green `#16a34a`, challenge orange `#ea580c`, fact dark green `#15803d`

## Known pitfalls (operational)

- Serve visualizations over `http://localhost:PORT` — Mermaid `htmlLabels` breaks on `file://`
- On Windows, write files with `newline='\n'` (CRLF breaks Mermaid)
- Escape `{`/`}` in LLM prompt templates used with Python `.format()`
- Articles > 15,000 chars are truncated; chunking is a known gap

## Sync obligations

- If `debate.proto`'s `LinkType` changes → update [`protocol.md`](protocol.md), this page, and the `paragraph-parsing` skill Section B **in the same change**.
- If the viewer's JSON schema changes → update this page and `extract_prompt.txt` together.
