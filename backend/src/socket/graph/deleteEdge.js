import { handleDeleteEdge } from '../../functions/socketFunctions/index.js';
export default function deleteEdge(io, socket, edge) {
    if (!socket.currentRoom) {
      return socket.emit('error', { message: 'Must join a room first' });
    }
    handleDeleteEdge(io, socket, { ...edge, room: socket.currentRoom });
 }
