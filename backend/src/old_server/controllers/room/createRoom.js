// ! Room Controller: Create Room
// * Creates a new room with initial owner and settings.
// ? Generates invite code for private rooms.
// TODO: Add validation for duplicate room names if needed.
import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Create a new room
 */
export async function createRoom(req, res) {
  try {
    const { name, description, isPublic = false, maxMembers = 50 } = req.body;
    const userId = req.user.userId;

    if (!name || name.trim().length === 0) {
      return res.status(400).json({ message: 'Room name is required' });
    }

    const room = new RoomModel({
      name: name.trim(),
      description: description?.trim() || '',
      owner: userId,
      members: [{
        user: userId,
        role: 'owner',
        joinedAt: new Date()
      }],
      isPublic,
      maxMembers: Math.min(Math.max(maxMembers, 1), 200)
    });

    if (!isPublic) {
      room.generateInviteCode();
    }

    await room.save();
    await room.populate('owner', 'email');
    await room.populate('members.user', 'email');

    res.status(201).json({
      message: 'Room created successfully',
      room
    });
  } catch (error) {
    console.error('Create room error:', error);
    res.status(500).json({ message: 'Failed to create room' });
  }
}
