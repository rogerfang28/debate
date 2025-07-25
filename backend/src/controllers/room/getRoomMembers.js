import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Get room members with usernames for challenge dropdown
 */
export async function getRoomMembers(req, res) {
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

    if (!room.hasMember(userId)) {
      return res.status(403).json({ message: 'Access denied - not a room member' });
    }

    await room.populate('members.user', 'email username');

    const members = room.members.map(member => ({
      id: member.user._id,
      email: member.user.email,
      username: member.user.username || member.user.email.split('@')[0],
      displayName: member.user.username || member.user.email,
      isCurrentUser: member.user._id.toString() === userId
    }));

    res.json({ members });
  } catch (error) {
    console.error('Get room members error:', error);
    res.status(500).json({ message: 'Failed to fetch room members' });
  }
}
