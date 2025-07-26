import { handleDeleteNode } from '../../functions/socketFunctions/index.js';
export default function deleteNode(io, socket, payload) {
  if (!socket.currentRoom) {
    return socket.emit('error', { message: 'Must join a room first' });
  }
  handleDeleteNode(io, socket, { ...payload, room: socket.currentRoom });
}
