import { socket } from '../../lib/socket';

/*
 Deletes a node from the graph.
 @param {string} nodeId - The ID of the node to delete.
*/
export function deleteNode(nodeId) {
  socket.emit('delete-node', { id: nodeId });
}
