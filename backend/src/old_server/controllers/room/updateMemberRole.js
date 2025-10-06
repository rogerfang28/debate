import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Update member role (owner only)
 */
export async function updateMemberRole(req, res) {
  try {
    const { roomId, userId: targetUserId } = req.params;
    const { role } = req.body;
    const userId = req.user.userId;

    if (!['member', 'admin'].includes(role)) {
      return res.status(400).json({ message: 'Invalid role' });
    }

    if (!mongoose.Types.ObjectId.isValid(roomId) || !mongoose.Types.ObjectId.isValid(targetUserId)) {
      return res.status(400).json({ message: 'Invalid room or user ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (room.owner.toString() !== userId) {
      return res.status(403).json({ message: 'Access denied - only owner can change roles' });
    }

    if (targetUserId === room.owner.toString()) {
      return res.status(400).json({ message: 'Cannot change owner role' });
    }

    const member = room.members.find(m => m.user.toString() === targetUserId);
    if (!member) {
      return res.status(404).json({ message: 'Member not found' });
    }

    member.role = role;
    await room.save();

    res.json({ message: 'Member role updated successfully' });
  } catch (error) {
    console.error('Update member role error:', error);
    res.status(500).json({ message: 'Failed to update member role' });
  }
}
