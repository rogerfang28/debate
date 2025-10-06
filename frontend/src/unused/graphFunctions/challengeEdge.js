// challengeEdge.js
import { socket } from '../../lib/socket.js';

/*
 * Challenge an edge in the graph
 * @param {string} edgeId - The ID of the edge to challenge
 * @param {string} responder - Username of the responding user
 * @param {string} reason - The reason for the challenge
 * @param {string} roomId - The room ID
 */
export function challengeEdge(edgeId, responder, reason, roomId) {
  console.log('Challenging edge:', { edgeId, responder, reason, roomId });
  
  socket.emit('challenge', {
    targetType: 'edge',
    targetId: edgeId,
    responder,
    challengeReason: reason,
    room: roomId
  });
}