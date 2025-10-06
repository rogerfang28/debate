// components/graphFunctions/challengeEdge.jsx
import { socket } from '../../lib/socket';

/*
 * Challenges an edge.
 * 
 * @param {string} source - Source node ID
 * @param {string} target - Target node ID
 * @param {string} relation - Type of connection (e.g. 'supports')
 * @param {string} challenger
 * @param {string} responder
 * @param {string} reason - Why this edge is being challenged
 * @param {string} roomId - The room ID where the challenge should be created
 */
export function challengeEdge(source, target, relation, challenger, responder, reason, roomId) {
  if (!source || !target || !relation || !reason) {
    console.warn('Missing edge challenge details');
    return;
  }

  socket.emit('challenge', {
    targetType: 'edge',
    target: { source, target, relation },
    challenger,
    responder,
    challengeReason: reason,
    room: roomId  // Fixed: backend expects 'room'
  });

  console.log(`📨 Edge challenge sent for ${source} → ${target} (${relation}) in room ${roomId}`);
}
