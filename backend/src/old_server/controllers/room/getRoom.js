// ! Room Controller: Get Room
// * Fetches details for a specific room.
// ? Checks membership and returns user role.
// TODO: Add audit logging for room access if needed.
import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Get specific room details
 */
export async function getRoom(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId)
      .populate('owner', 'email username')
      .populate('members.user', 'email username');

    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (!room.hasMember(userId)) {
      return res.status(403).json({ message: 'Access denied - not a room member' });
    }

    const userRole = room.getMemberRole(userId);
    res.json({
      room: {
        ...room.toObject(),
        userRole,
        memberCount: room.members.length
      }
    });
  } catch (error) {
    console.error('Get room error:', error);
    res.status(500).json({ message: 'Failed to fetch room' });
  }
}
