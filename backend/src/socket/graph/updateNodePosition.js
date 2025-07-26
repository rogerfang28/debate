import { handleUpdateNodePosition } from '../../functions/socketFunctions/index.js';

export default function updateNodePosition(io, socket, payload) {
  if (!socket.currentRoom) {
    return socket.emit('error', { message: 'Must join a room first' });
  }
  handleUpdateNodePosition(io, socket, { ...payload, room: socket.currentRoom });
}