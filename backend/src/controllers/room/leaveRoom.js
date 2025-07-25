import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Leave room
 */
export async function leaveRoom(req, res) {
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

    if (room.owner.toString() === userId) {
      return res.status(400).json({ message: 'Room owner cannot leave. Transfer ownership or delete room.' });
    }

    if (!room.hasMember(userId)) {
      return res.status(400).json({ message: 'Not a member of this room' });
    }

    room.members = room.members.filter(
      member => member.user.toString() !== userId
    );

    await room.save();

    res.json({ message: 'Left room successfully' });
  } catch (error) {
    console.error('Leave room error:', error);
    res.status(500).json({ message: 'Failed to leave room' });
  }
}
