import { socket } from '../../lib/socket';

/*
 Deletes an edge between two nodes.
 @param {string} sourceId - The source node ID.
 @param {string} targetId - The target node ID.
*/
export function deleteEdge(sourceId, targetId) {
  socket.emit('delete-edge', { source: sourceId, target: targetId });
}
