# 🛰️ Real‑Time Layer (`src/socket/`)

This folder contains **all WebSocket / Socket.IO code** for the project. It plays the same role for real‑time traffic that Express controllers play for HTTP.

| Transport     | Entry file            | Feature routers          | Purpose        |
| ------------- | --------------------- | ------------------------ | -------------- |
| **HTTP**      | `src/app.ts`          | `src/routes/…`           | REST endpoints |
| **WebSocket** | `src/socket/index.js` | `src/socket/<feature>/…` | Real‑time API  |

---

## Folder map

```text
socket/
├── index.js            # Bootstraps sockets; attaches middleware & feature routers
├── utils.js            # Transport‑agnostic helpers (emitToUser, trackSocket, …)
│
├── auth.js             # (Optional) extra auth / namespace logic
│
├── room/               # Join / leave debate rooms
│   ├── index.js
│   ├── joinRoom.js
│   └── leaveRoom.js
│
├── chat/               # Room chat
│   ├── index.js
│   └── sendMessage.js
│
├── graph/              # Debate‑graph editing & challenges
│   ├── index.js
│   ├── addNode.js
│   ├── deleteNode.js
│   ├── addEdge.js
│   ├── deleteEdge.js
│   ├── updateNodePosition.js
│   ├── challenge.js
│   └── respondChallenge.js
│
└── socketHandler.js    # ⚠️ Legacy monolith — delete when fully migrated
```

---

## Event flow in 5 steps

1. **`socket/index.js`** authenticates the socket (JWT) and calls each feature router.
2. A feature router (e.g. `graph/index.js`) wires event names to handlers:

   ```js
   socket.on('add-node', p => addNode(io, socket, p));
   ```
3. The **handler** (`graph/addNode.js`) validates input & permissions.
4. It delegates business logic to a **service** (`services/graph/addNode.js`).
5. It broadcasts results back to the room with `io.to(roomId).emit(...)`.

---

## Adding a new real‑time feature

1. Create a folder `socket/<feature>/`.
2. Write one file per event (`doX.js`, `doY.js`).
3. Add an `index.js` in that folder that registers the events.
4. Import that router in `socket/index.js` and call it inside the `connection` callback.
5. Unit‑test each handler by mocking `io` and `socket`.

---

## Testing snippet

```js
import { Server } from 'socket.io';
import { createServer } from 'http';
import registerSockets from '../../socket/index.js';

it('broadcasts add‑node', done => {
  const httpServer = createServer();
  const io = new Server(httpServer);
  registerSockets(io);

  io.on('connection', client => {
    client.currentRoom = 'r1';                 // stub join
    client.on('node:add', msg => {
      expect(msg.label).toBe('Hello');
      done();
    });
    client.emit('add-node', { id: 'n1', label: 'Hello' });
  });

  httpServer.listen(() => {
    const { port } = httpServer.address();
    require('socket.io-client')(`http://localhost:${port}`, {
      auth: { token: 'TEST_JWT' }
    });
  });
});
```

---

## Migration status checklist

| Feature | Old path (`functions/socketFunctions`) | New path (`socket/<feature>`) | Done |
| ------- | -------------------------------------- | ----------------------------- | ---- |
| Graph   | removed                                | `socket/graph/*`              | ✅    |
| Chat    | n/a                                    | `socket/chat/*`               | ✅    |
| Rooms   | part of old monolith                   | `socket/room/*`               | ✅    |

When every feature is ✅, delete **`socket/socketHandler.js`** and the now‑unused `functions/socketFunctions` folder.

---

Happy hacking — and keep your real‑time code tidy! 🚀
