// ! Room Controller: Get Rooms
// * Fetches all rooms for the authenticated user.
// ? Includes user role and member count metadata.
// TODO: Add pagination if room count is large.
import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Get all rooms for the authenticated user
 */
export async function getRooms(req, res) {
  try {
    const userId = req.user.userId;

    const rooms = await RoomModel.find({
      'members.user': userId
    })
    .populate('owner', 'email username')
    .populate('members.user', 'email username')
    .sort({ updatedAt: -1 });

    const roomsWithMetadata = rooms.map(room => {
      const userRole = room.getMemberRole(userId);
      return {
        ...room.toObject(),
        userRole,
        memberCount: room.members.length
      };
    });

    res.json({ rooms: roomsWithMetadata });
  } catch (error) {
    console.error('Get rooms error:', error);
    res.status(500).json({ message: 'Failed to fetch rooms' });
  }
}
