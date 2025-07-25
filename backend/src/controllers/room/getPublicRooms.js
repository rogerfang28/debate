import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Get public rooms for browsing (no authentication required)
 */
export async function getPublicRooms(req, res) {
  try {
    const { page = 1, limit = 20, search = '' } = req.query;
    const skip = (page - 1) * limit;

    const filter = { isPublic: true };
    if (search.trim()) {
      filter.$or = [
        { name: { $regex: search, $options: 'i' } },
        { description: { $regex: search, $options: 'i' } }
      ];
    }

    const rooms = await RoomModel.find(filter)
      .select('name description createdAt members')
      .populate('owner', 'email username')
      .sort({ createdAt: -1 })
      .skip(skip)
      .limit(parseInt(limit));

    const total = await RoomModel.countDocuments(filter);

    const roomsWithCounts = await Promise.all(
      rooms.map(async (room) => {
        const memberCount = room.members?.length || 0;
        return {
          _id: room._id,
          name: room.name,
          description: room.description,
          owner: room.owner,
          createdAt: room.createdAt,
          memberCount
        };
      })
    );

    res.json({
      rooms: roomsWithCounts,
      pagination: {
        page: parseInt(page),
        limit: parseInt(limit),
        total,
        pages: Math.ceil(total / limit)
      }
    });
  } catch (error) {
    console.error('Get public rooms error:', error);
    res.status(500).json({ message: 'Failed to fetch public rooms' });
  }
}
