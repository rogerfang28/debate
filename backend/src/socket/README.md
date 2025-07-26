# 🛰️ Real‑Time Layer (`src/socket/`)

This folder holds **all WebSocket / Socket.IO code** for the project.  
It mirrors the way we treat HTTP:

| Transport | Entry file | Feature routers | Purpose |
|-----------|------------|-----------------|---------|
| **HTTP**  | `src/app.js` (Express) | `routes/` | REST endpoints |
| **WS**    | `src/socket/index.js`   | `socket/<feature>/` | Real‑time events |

The sockets layer is split by **feature/bounded‑context**, so you can work on
graph logic without touching chat or room membership.

---

## Folder map

