import { Server } from 'socket.io';
import { allowedOrigins } from './cors.js';

export default function createSocketServer(server) {
  return new Server(server, {
    cors: {
      origin: allowedOrigins,
      methods: ["GET", "POST"],
      credentials: true
    }
  });
}
