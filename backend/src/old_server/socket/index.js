import jwt from 'jsonwebtoken';
import { initIO, trackSocket, untrackSocket } from './utils.js';

import registerRoomEvents  from './room/index.js';
import registerGraphEvents from './graph/index.js';
import registerChatEvents  from './chat/index.js';

const JWT_SECRET = process.env.JWT_SECRET;

/** Attach this once in server.js */
export default function registerSockets(io) {
  initIO(io);                           // makes io available to utils.js

  /* ── connection‑level auth ───────────────────────────── */
  io.use((socket, next) => {
    try {
      const token =
        socket.handshake.auth.token ||
        socket.handshake.headers.authorization?.split(' ')[1];

      if (!token) return next(new Error('Authentication token required'));

      const { userId, email } = jwt.verify(token, JWT_SECRET);
      socket.userId    = userId;
      socket.userEmail = email;
      next();
    } catch (err) {
      next(new Error('Authentication failed'));
    }
  });

  /* ── per‑socket setup ───────────────────────────────── */
  io.on('connection', socket => {
    trackSocket(socket.userId, socket.id);

    /* register feature routers */
    registerRoomEvents (io, socket);
    registerGraphEvents(io, socket);
    registerChatEvents (io, socket);

    socket.on('disconnect', () => {
      untrackSocket(socket.userId, socket.id);
      // fire leaveRoom if needed
      if (socket.currentRoom) {
        socket.to(socket.currentRoom).emit('user-left-room', {
          userId: socket.userId, userEmail: socket.userEmail,
          roomId: socket.currentRoom
        });
      }
    });
  });
}
