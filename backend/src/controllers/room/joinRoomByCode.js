import { RoomModel } from '../../models/Room.js';
import mongoose from 'mongoose';

/**
 * Join room using invite code
 */
export async function joinRoomByCode(req, res) {
  try {
    const { inviteCode } = req.params;
    const userId = req.user.userId;

    const room = await RoomModel.findOne({ inviteCode })
      .populate('owner', 'email')
      .populate('members.user', 'email');

    if (!room) {
      return res.status(404).json({ message: 'Invalid invite code' });
    }

    if (room.hasMember(userId)) {
      return res.status(400).json({ message: 'Already a member of this room' });
    }

    if (room.members.length >= room.maxMembers) {
      return res.status(400).json({ message: 'Room is at maximum capacity' });
    }

    room.members.push({
      user: userId,
      role: 'member',
      joinedAt: new Date()
    });

    await room.save();
    await room.populate('members.user', 'email');

    res.json({
      message: 'Successfully joined room',
      room: {
        ...room.toObject(),
        userRole: 'member',
        memberCount: room.members.length
      }
    });
  } catch (error) {
    console.error('Join room error:', error);
    res.status(500).json({ message: 'Failed to join room' });
  }
}
