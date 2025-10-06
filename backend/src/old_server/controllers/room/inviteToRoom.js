import { RoomModel } from '../../models/Room.js';
import { UserModel } from '../../models/User.js';
import mongoose from 'mongoose';

/**
 * Invite user to room by email
 */
export async function inviteToRoom(req, res) {
  try {
    const { roomId } = req.params;
    const { email } = req.body;
    const userId = req.user.userId;

    if (!email) {
      return res.status(400).json({ message: 'Email is required' });
    }

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    const userRole = room.getMemberRole(userId);
    if (!userRole) {
      return res.status(403).json({ message: 'Access denied - not a room member' });
    }

    if (userRole === 'member' && !room.settings.allowMemberInvites) {
      return res.status(403).json({ message: 'Access denied - members cannot send invites' });
    }

    const targetUser = await UserModel.findOne({ email });
    if (!targetUser) {
      return res.status(404).json({ message: 'User not found' });
    }

    if (room.hasMember(targetUser._id)) {
      return res.status(400).json({ message: 'User is already a member' });
    }

    if (room.members.length >= room.maxMembers) {
      return res.status(400).json({ message: 'Room is at maximum capacity' });
    }

    if (!room.inviteCode) {
      room.generateInviteCode();
      await room.save();
    }

    res.json({
      message: 'Invite ready',
      inviteCode: room.inviteCode,
      inviteUrl: `/join/${room.inviteCode}`
    });
  } catch (error) {
    console.error('Invite to room error:', error);
    res.status(500).json({ message: 'Failed to send invite' });
  }
}
