// components/graphFunctions/challengeNode.jsx
import { socket } from '../../lib/socket';

/*
 * Challenges a node.
 * 
 * @param {string} nodeId - ID of the node being challenged
 * @param {string} challenger
 * @param {string} responder
 * @param {string} reason - Why this node is being challenged
 */
export function challengeNode(nodeId, challenger, responder, reason) {
  if (!nodeId || !reason) {
    console.warn('Missing nodeId or reason');
    return;
  }

  socket.emit('challenge', {
    targetType: 'node',
    target: nodeId,
    challenger,
    responder,
    challengeReason: reason
  });

  console.log(`📨 Node challenge sent for ${nodeId}`);
}
