              ┌─────────────┐
   Browser ──►│   ROUTES    │  (maps URL → controller)
              └─────────────┘
              ┌─────────────┐
              │ CONTROLLERS │  (I/O glue only)
              └─────────────┘
              ┌─────────────┐   ← **“main logic” lives here**
              │  SERVICES   │  business rules, multi‑step ops
              └─────────────┘
              ┌─────────────┐
              │   MODELS    │  DB schemas + simple CRUD helpers
              └─────────────┘
              ┌─────────────┐
              │   HELPERS   │  pure utilities
              └─────────────┘

Node json format:

Edge json format:
