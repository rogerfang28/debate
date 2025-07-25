import { FriendModel } from '../../models/Friend.js';
import { RoomModel } from '../../models/Room.js';
import { MessageModel } from '../../models/Message.js';

/**
 * Send room invitation via chat
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function sendRoomInvitation(req, res) {
  try {
    const { friendId, roomId } = req.params;
    const currentUserId = req.user.userId;

    // Verify friendship
    const friendship = await FriendModel.findOne({
      $or: [
        { requester: currentUserId, recipient: friendId },
        { requester: friendId, recipient: currentUserId }
      ],
      status: 'accepted'
    });

    if (!friendship) {
      return res.status(403).json({ message: 'You can only invite friends' });
    }

    // Verify room access and get room details
    const room = await RoomModel.findOne({
      _id: roomId,
      'members.user': currentUserId,
      'members.role': { $in: ['owner', 'admin'] }
    });

    if (!room) {
      return res.status(403).json({ message: 'You can only invite to rooms you own or admin' });
    }

    // Generate invite code if needed
    if (!room.inviteCode) {
      room.inviteCode = Math.random().toString(36).substring(2, 15) + 
                       Math.random().toString(36).substring(2, 15);
      await room.save();
    }

    // Create invitation message
    const message = new MessageModel({
      sender: currentUserId,
      recipient: friendId,
      content: `🏠 You've been invited to join "${room.name}"! Click to join this debate room.`,
      messageType: 'room_invite',
      roomInvite: {
        roomId: room._id,
        roomName: room.name,
        inviteCode: room.inviteCode
      }
    });

    await message.save();
    await message.populate('sender', 'username email');

    res.status(201).json(message);
  } catch (error) {
    console.error('Send room invitation error:', error);
    res.status(500).json({ message: 'Failed to send room invitation' });
  }
}
