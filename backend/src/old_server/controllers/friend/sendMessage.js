// * -------------------------------------------------------------
// * Friend Controller: sendMessage.js
// ? Handles sending a message to a friend (text or room invite).
// ! Only allows messaging accepted friends.
// TODO: Add message delivery receipts or notifications.
// * -------------------------------------------------------------
import { FriendModel } from '../../models/Friend.js';
import { MessageModel } from '../../models/Message.js';
import { emitToUser } from '../../socket/utils.js';

/**
 * Send message to friend
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function sendMessage(req, res) {
  try {
    // ? Get friendId, message content, and current user
    const { friendId } = req.params;
    const { content, messageType = 'text', roomInvite } = req.body;
    const currentUserId = req.user.userId;

    // ! Require non-empty message content
    if (!content || content.trim().length === 0) {
      return res.status(400).json({ message: 'Message content is required' });
    }

    // * Verify friendship exists
    const friendship = await FriendModel.findOne({
      $or: [
        { requester: currentUserId, recipient: friendId },
        { requester: friendId, recipient: currentUserId }
      ],
      status: 'accepted'
    });

    if (!friendship) {
      return res.status(403).json({ message: 'You can only message friends' });
    }

    // * Create message
    const message = new MessageModel({
      sender: currentUserId,
      recipient: friendId,
      content: content.trim(),
      messageType,
      ...(roomInvite && { roomInvite })
    });

    await message.save();
    await message.populate('sender', 'username email');

    // * Emit real-time message to recipient
    emitToUser(friendId, 'new-message', message); // Main event
    emitToUser(friendId, 'new-message-chatfloat', message); // ChatFloat UI
    emitToUser(friendId, 'new-message-modal', message); // Modal UI

    // * Respond with message object
    res.status(201).json(message);
  } catch (error) {
    // ! Log and handle unexpected errors
    console.error('Send message error:', error);
    res.status(500).json({ message: 'Failed to send message' });
  }
}
