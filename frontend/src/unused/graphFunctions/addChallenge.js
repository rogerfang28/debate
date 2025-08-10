// components/graphFunctions/addChallenge.jsx
import { socket } from '../../lib/socket';

/*
 * Emits a challenge for an edge (based on source and target nodes).
 * 
 * @param {string} sourceId - Source node ID
 * @param {string} targetId - Target node ID  
 * @param {string} challenger
 * @param {string} responder
 * @param {string} reason - The reason for the challenge
 * @param {string} roomId - The room ID where the challenge should be created
 */
export function addChallenge(sourceId, targetId, challenger, responder, reason, roomId) {
  if (!reason || reason.trim() === '') {
    console.warn('Challenge reason is required.');
    return;
  }

  socket.emit('challenge', {
    targetType: 'edge',
    target: { source: sourceId, target: targetId },
    challenger,
    responder,
    challengeReason: reason,
    room: roomId  // Fixed: backend expects 'room'
  });

  console.log(`📨 Edge challenge sent for ${sourceId}->${targetId} in room ${roomId}: ${reason}`);
}
