import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Generate new invite code
 */
export async function generateInviteCode(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    const userRole = room.getMemberRole(userId);
    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Access denied - insufficient permissions' });
    }

    const newCode = room.generateInviteCode();
    await room.save();

    res.json({
      message: 'New invite code generated',
      inviteCode: newCode,
      inviteUrl: `/join/${newCode}`
    });
  } catch (error) {
    console.error('Generate invite code error:', error);
    res.status(500).json({ message: 'Failed to generate invite code' });
  }
}
