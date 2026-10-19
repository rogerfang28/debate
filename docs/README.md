# Debate Tool — Documentation

> **Structured Logical Debate Platform.** Make it easiest to see the truth and hardest to spread lies, by structurally exposing holes in false claims so they can be pointed out and debunked.

- **Repo:** https://github.com/rogerfang28/debate
- **Live demo (WIP):** https://debate-spxg.onrender.com/

---

## How this documentation is organized

Docs are **layered by depth**. The number prefix on a file tells you how deep it goes. Read top-down for a full onboarding, or jump straight to your layer.

| Layer | Prefix | Audience | Question it answers |
|-------|--------|----------|---------------------|
| **0 — Overview** | `00-` | Anyone (non-technical) | *What is this and why does it exist?* |
| **1 — Concepts** | `10-`, `11-` | Users, designers, contributors | *How does the system behave? What must it handle?* |
| **2 — Architecture** | `20-` | Developers (orientation) | *What are the parts and how do they fit together?* |
| **3 — Reference** | `30-reference/` | Developers (implementation) | *How does this specific component work in code?* |
| **4 — Status** | `40-` | Everyone | *Where is the project now and where is it going?* |
| **Meta** | `meta/` | Doc maintainers & agents | *How do I keep these docs correct?* |

## File map

```
docs/
├── README.md                  ← you are here (navigation hub)
├── 00-overview.md             Layer 0 — mission, problem, elevator pitch
├── 10-concepts.md             Layer 1 — the challenge system & core mechanics
├── 11-design-cases.md         Layer 1 — the 11 scenario requirements (spec)
├── 20-architecture.md         Layer 2 — components, tech stack, data flow
├── 30-reference/              Layer 3 — per-component deep dives
│   ├── backend.md               C++ server & challenge-collapse algorithm
│   ├── frontend.md              React/TS server-driven UI
│   ├── protocol.md              Protocol Buffers contract
│   ├── data-model.md            SQLite schema & debate graph model
│   └── pipeline-parsing.md      Article → claim-tree extraction pipeline
├── 40-roadmap.md              Layer 4 — current status & planned work
└── meta/
    ├── DOC-GUIDE.md           Rules for maintaining these docs (READ FIRST if editing)
    ├── TEMPLATE-reference.md  Copy this to add a new Layer-3 doc
    └── CHANGELOG.md           Log of documentation changes
```

## Reading paths

- **"What is this project?"** → `00-overview.md`
- **"How do debates actually work?"** → `10-concepts.md`, then `11-design-cases.md`
- **"I'm about to write code."** → `20-architecture.md`, then the relevant file in `30-reference/`
- **"I'm an agent about to modify code or docs."** → `meta/DOC-GUIDE.md` first, always.

## Ground rules (summary — full rules in `meta/DOC-GUIDE.md`)

1. Every doc has a frontmatter header (`layer`, `status`, `last-updated`).
2. Information lives at exactly **one** layer; other layers **link** to it, never duplicate it.
3. Code changes that alter behavior **must** update the matching Layer-3 doc in the same change.
4. New components get a new file in `30-reference/` from the template — never bolt deep detail onto Layer 0–2 docs.
