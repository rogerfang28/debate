import { handleAddEdge } from '../../functions/socketFunctions/index.js';
export default function addEdge(io, socket, edge) { 
  if (!socket.currentRoom) {
    return socket.emit('error', { message: 'Must join a room first' });
  }
  handleAddEdge(io, socket, { ...edge, room: socket.currentRoom });
 }
