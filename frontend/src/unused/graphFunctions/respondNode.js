// components/graphFunctions/respondChallenge.jsx
import { socket } from '../../lib/socket';

/**
 * Responds to a challenge (accept or deny).
 * 
 * @param {string} challengeId - MongoDB _id of the challenge.
 * @param {string} user - The user responding to the challenge.
 * @param {'accepted'|'denied'} action - Your response.
 * @param {string} roomId - The room ID where the challenge response should be created.
 * @param {string} [reason] - Optional explanation for your response.
 */
export function respondChallenge(challengeId, user, action, roomId, reason = '') {
  if (!['accepted', 'denied'].includes(action)) {
    console.warn('Invalid challenge response action:', action);
    return;
  }

  socket.emit('respond-to-challenge', {
    challengeId,
    user,
    action,
    reason,
    room: roomId  // Fixed: backend expects 'room'
  });

  console.log(`📨 Responded to challenge ${challengeId}: ${action} in room ${roomId}`);
}
