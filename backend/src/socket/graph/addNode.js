import { handleAddNode } from '../../functions/socketFunctions/index.js';

export default function addNode(io, socket, node) {
  if (!socket.currentRoom) {
    return socket.emit('error', { message: 'Must join a room first' });
  }
  handleAddNode(io, socket, { ...node, room: socket.currentRoom });
}
