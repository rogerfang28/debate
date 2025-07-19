import { socket } from '../../lib/socket';

/*
 Adds a new node to the graph
 @param {string} node_label - The label/text content of the node
 @param {string} roomId - The room ID where the node should be added
*/
export function addNode(node_label, roomId) {
  // Generate random position around center for better initial layout
  const randomX = (Math.random() - 0.5) * 400; // Random between -200 and 200
  const randomY = (Math.random() - 0.5) * 400; // Random between -200 and 200
  
  const newNode = {
    id: Date.now().toString(),  // Temporary unique ID based on timestamp
    label: node_label,
    metadata: {
      createdAt: new Date().toISOString()
    },
    x: randomX,
    y: randomY,
    room: roomId  // Fixed: backend expects 'room' not 'roomId'
  };

  socket.emit('add-node', newNode);

  return newNode; // in case you want to use the ID to link right after
}