import { socket } from '../../lib/socket';

/*
 Deletes an edge between two nodes.
 @param {string} sourceId - The source node ID.
 @param {string} targetId - The target node ID.
 @param {string} roomId - The room ID where the edge should be deleted.
*/
export function deleteEdge(sourceId, targetId, roomId) {
  socket.emit('delete-edge', { source: sourceId, target: targetId, room: roomId });  // Fixed: backend expects 'room'
}
