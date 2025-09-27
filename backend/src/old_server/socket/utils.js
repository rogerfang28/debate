/** Utilities shared by every socket feature */

const userSockets = new Map();   // userId → Set<socketId>
let io;                          // filled once by initIO()

export function initIO(ioInstance) {
  io = ioInstance;
}

/* ── user ↔ socket tracking ────────────────────────────────────────────── */
export function trackSocket(userId, socketId) {
  if (!userSockets.has(userId)) userSockets.set(userId, new Set());
  userSockets.get(userId).add(socketId);
}

export function untrackSocket(userId, socketId) {
  const set = userSockets.get(userId);
  if (!set) return;
  set.delete(socketId);
  if (set.size === 0) userSockets.delete(userId);
}

/* ── one‑shot emit helper (DMs, friend requests, …) ────────────────────── */
export function emitToUser(userId, event, data) {
  const ids = userSockets.get(userId);
  if (!ids || ids.size === 0) return;

  ids.forEach(id => {
    const s = io.sockets.sockets.get(id);
    if (s) s.emit(event, data);
    else   ids.delete(id); // stale socket
  });
}
