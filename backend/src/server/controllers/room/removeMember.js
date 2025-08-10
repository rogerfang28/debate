import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Remove member from room
 */
export async function removeMember(req, res) {
  try {
    const { roomId, userId: targetUserId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId) || !mongoose.Types.ObjectId.isValid(targetUserId)) {
      return res.status(400).json({ message: 'Invalid room or user ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    const userRole = room.getMemberRole(userId);
    const targetRole = room.getMemberRole(targetUserId);

    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Access denied - insufficient permissions' });
    }

    if (targetUserId === room.owner.toString()) {
      return res.status(400).json({ message: 'Cannot remove room owner' });
    }

    if (userRole === 'admin' && targetRole === 'admin') {
      return res.status(403).json({ message: 'Admins cannot remove other admins' });
    }

    room.members = room.members.filter(
      member => member.user.toString() !== targetUserId
    );

    await room.save();

    res.json({ message: 'Member removed successfully' });
  } catch (error) {
    console.error('Remove member error:', error);
    res.status(500).json({ message: 'Failed to remove member' });
  }
}
