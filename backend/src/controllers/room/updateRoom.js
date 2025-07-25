// ! Room Controller: Update Room
// * Updates room details (name, description, settings).
// ? Only owner/admin can update room.
// TODO: Add validation for settings changes.
import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Update room details (owner/admin only)
 */
export async function updateRoom(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;
    const { name, description, isPublic, maxMembers, settings } = req.body;

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

    if (name !== undefined) room.name = name.trim();
    if (description !== undefined) room.description = description.trim();
    if (isPublic !== undefined) room.isPublic = isPublic;
    if (maxMembers !== undefined) {
      room.maxMembers = Math.min(Math.max(maxMembers, 1), 200);
    }
    if (settings !== undefined) {
      room.settings = { ...room.settings, ...settings };
    }

    await room.save();
    await room.populate('owner', 'email');
    await room.populate('members.user', 'email');

    res.json({
      message: 'Room updated successfully',
      room
    });
  } catch (error) {
    console.error('Update room error:', error);
    res.status(500).json({ message: 'Failed to update room' });
  }
}
