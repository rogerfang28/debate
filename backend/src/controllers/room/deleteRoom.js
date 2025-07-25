// ! Room Controller: Delete Room
// * Deletes a room and all associated nodes/edges.
// ? Only the room owner can perform this action.
// TODO: Add audit logging for deletions if needed.
import { RoomModel } from '../../models/Room.js';
import { NodeModel } from '../../models/Node.js';
import { EdgeModel } from '../../models/Edge.js';
import mongoose from 'mongoose';

/**
 * Delete room (owner only)
 */
export async function deleteRoom(req, res) {
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

    if (room.owner.toString() !== userId) {
      return res.status(403).json({ message: 'Access denied - only room owner can delete' });
    }

    await Promise.all([
      NodeModel.deleteMany({ room: roomId }),
      EdgeModel.deleteMany({ room: roomId }),
      RoomModel.findByIdAndDelete(roomId)
    ]);

    res.json({ message: 'Room deleted successfully' });
  } catch (error) {
    console.error('Delete room error:', error);
    res.status(500).json({ message: 'Failed to delete room' });
  }
}
