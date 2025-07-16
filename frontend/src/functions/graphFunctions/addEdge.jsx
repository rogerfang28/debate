import { socket } from '../../lib/socket';

/*
 Adds a new edge between two nodes.
 
 @param {string} node_from - ID of the source node.
 @param {string} node_to - ID of the target node.
 @param {string} connection - The type of connection (e.g., "supports", "contradicts").
 @param {string} user_who_made_it - ID or name of the user who added the edge.
 @param {string} time_made - ISO timestamp (can default to now).
 */
export function addEdge(node_from, node_to, connection, user_who_made_it, time_made = new Date().toISOString()) {
  const newEdge = {
    source: node_from,
    target: node_to,
    metadata: {
      relation: connection,
      createdBy: user_who_made_it,
      createdAt: time_made
    }
  };

  socket.emit('add-edge', newEdge);
}