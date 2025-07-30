import { Server } from 'socket.io';
import { allowedOrigins } from './cors.js';

/**
 * Creates and configures a Socket.IO server instance.
 * @param {import('http').Server} server - The HTTP server instance.
 * @returns {import('socket.io').Server} The configured Socket.IO server.
 */
export default function createSocketServer(server) {
  return new Server(server, {
    cors: {
      origin: allowedOrigins,
      methods: ["GET", "POST"],
      credentials: true
    }
  });
}
