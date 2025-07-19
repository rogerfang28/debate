import { socket } from '../../lib/socket';

/*
 Deletes a node from the graph.
 @param {string} nodeId - The ID of the node to delete.
 @param {string} roomId - The room ID where the node should be deleted.
*/
export function deleteNode(nodeId, roomId) {
  socket.emit('delete-node', { id: nodeId, room: roomId });  // Fixed: backend expects 'room'
}
