
// respondChallenge.js
import { handleChallengeResponse } from '../../functions/socketFunctions/respondChallenge.js';
export default function respond(io, socket, payload) {
  if (!socket.currentRoom) return socket.emit('error', { message: 'Join a room' });
  handleChallengeResponse(io, socket, { ...payload, room: socket.currentRoom });
}