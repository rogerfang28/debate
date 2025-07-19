// components/graphFunctions/challengeNode.jsx
import { socket } from '../../lib/socket';

/*
 * Challenges a node.
 * 
 * @param {string} nodeId - ID of the node being challenged
 * @param {string} responder - Username of the responding user
 * @param {string} reason - Why this node is being challenged
 * @param {string} roomId - The room ID where the challenge should be created
 */
export function challengeNode(nodeId, responder, reason, roomId) {
  console.log('🎯 challengeNode called with:', { nodeId, responder, reason, roomId });
  
  if (!nodeId || !reason) {
    console.warn('❌ Missing nodeId or reason');
    return;
  }

  console.log('📡 Socket connected?', socket.connected);
  console.log('📨 Emitting challenge event...');

  socket.emit('challenge', {
    targetType: 'node',
    targetId: nodeId,
    responder,
    challengeReason: reason,
    room: roomId
  });

  console.log(`✅ Node challenge sent for ${nodeId} in room ${roomId}`);
}
