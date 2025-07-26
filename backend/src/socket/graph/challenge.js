import { handleChallenge } from '../../functions/socketFunctions/addChallenge.js';

export default function challenge(io, socket, payload) {
  if (!socket.currentRoom) return socket.emit('error', { message: 'Join a room' });
  handleChallenge(io, socket, { ...payload, room: socket.currentRoom });
}