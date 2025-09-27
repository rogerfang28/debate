import { ChallengeModel } from '../../models/Challenge.js';
import { RoomModel } from '../../models/Room.js';
import { NodeModel } from '../../models/Node.js';
import { EdgeModel } from '../../models/Edge.js';
import { emitRoomGraph } from './utils.js';

export default async function respond(io, socket, { challengeId, action, reason, room: roomId }) {
  try {
    // Verify room access
    const room = await RoomModel.findById(roomId);
    if (!room) {
      socket.emit('error', { message: 'Room not found' });
      return;
    }

    if (!room.hasMember(socket.userId)) {
      socket.emit('error', { message: 'Access denied - not a room member' });
      return;
    }

    // Find and validate challenge
    const challenge = await ChallengeModel.findById(challengeId).populate('challenger', 'email username');
    if (!challenge) {
      socket.emit('error', { message: 'Challenge not found' });
      return;
    }

    if (challenge.room.toString() !== roomId) {
      socket.emit('error', { message: 'Challenge not in this room' });
      return;
    }

    if (challenge.responder.toString() !== socket.userId) {
      socket.emit('error', { message: 'Access denied - you are not the responder' });
      return;
    }

    if (challenge.status !== 'pending') {
      socket.emit('error', { message: 'Challenge is no longer pending' });
      return;
    }

    if (!['accepted', 'denied'].includes(action)) {
      socket.emit('error', { message: 'Invalid action' });
      return;
    }

    if (!reason || !reason.trim()) {
      socket.emit('error', { message: 'Response reason is required' });
      return;
    }

    // Update challenge
    challenge.status = action;
    challenge.resolutionReason = reason || '';
    await challenge.save();

    // If challenge is accepted, remove the challenged node/edge
    if (action === 'accepted') {
      console.log(`🎯 Challenge accepted, attempting to delete ${challenge.targetType} with ID: ${challenge.targetId}`);
      
      try {
        if (challenge.targetType === 'node') {
          // Try to delete by custom id first, then by _id
          console.log('🔍 Searching for node with custom id:', challenge.targetId);
          let nodeDeleted = await NodeModel.findOneAndDelete({ id: challenge.targetId });
          
          if (!nodeDeleted) {
            console.log('🔍 Not found by custom id, trying MongoDB _id:', challenge.targetId);
            nodeDeleted = await NodeModel.findByIdAndDelete(challenge.targetId);
          }
          
          if (nodeDeleted) {
            console.log('✅ Node found and deleted:', nodeDeleted);
            // Delete all connected edges using both id and _id
            const deletedEdges = await EdgeModel.deleteMany({
              $or: [
                { source: challenge.targetId },
                { target: challenge.targetId },
                { source: nodeDeleted.id },
                { target: nodeDeleted.id }
              ]
            });
            console.log(`🗑️ Node ${challenge.targetId} removed due to accepted challenge`);
            console.log(`🗑️ ${deletedEdges.deletedCount} connected edges also removed`);
          } else {
            console.warn(`⚠️ Node ${challenge.targetId} not found for deletion`);
          }
        } else if (challenge.targetType === 'edge') {
          // Try to delete edge by custom id first, then by _id
          console.log('🔍 Searching for edge with custom id:', challenge.targetId);
          let edgeDeleted = await EdgeModel.findOneAndDelete({ id: challenge.targetId });
          
          if (!edgeDeleted) {
            console.log('🔍 Not found by custom id, trying MongoDB _id:', challenge.targetId);
            edgeDeleted = await EdgeModel.findByIdAndDelete(challenge.targetId);
          }
          
          if (edgeDeleted) {
            console.log('✅ Edge found and deleted:', edgeDeleted);
            console.log(`🗑️ Edge ${challenge.targetId} removed due to accepted challenge`);
          } else {
            console.warn(`⚠️ Edge ${challenge.targetId} not found for deletion`);
          }
        }
      } catch (deleteError) {
        console.error('❌ Error removing challenged item:', deleteError);
        // Continue even if deletion fails
      }
    }

    console.log(`✅ Challenge ${action} by ${socket.userEmail} in room: ${room.name} (challenger: ${challenge.challenger.username || challenge.challenger.email})`);
    
    // Notify the challenger about the response
    const challengerNotification = {
      type: 'challenge-response',
      challengeId: challenge._id,
      targetType: challenge.targetType,
      targetId: challenge.targetId,
      action: action,
      responder: socket.userEmail,
      reason: challenge.resolutionReason,
      timestamp: new Date()
    };
    
    // Send notification to all room members
    io.to(roomId).emit('challenge-response-notification', challengerNotification);
    
    await emitRoomGraph(io, roomId);
    
    socket.emit('challenge-responded', { challengeId, action });
  } catch (err) {
    console.error('❌ Challenge response failed:', err);
    socket.emit('error', { message: 'Failed to respond to challenge' });
  }
}
