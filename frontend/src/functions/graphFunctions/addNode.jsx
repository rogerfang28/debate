import { socket } from '../../lib/socket';

/*
 Adds a new node to the graph
 @param {string} node_label - The label/text content of the node
*/
export function addNode(node_label) {
  const newNode = {
    id: Date.now().toString(),  // Temporary unique ID based on timestamp
    label: node_label,
    metadata: {
      createdAt: new Date().toISOString()
    }
  };

  socket.emit('add-node', newNode);

  return newNode; // in case you want to use the ID to link right after
}