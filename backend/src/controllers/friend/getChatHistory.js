// * -------------------------------------------------------------
// * Friend Controller: getChatHistory.js
// ? Retrieves chat history between the user and a friend.
// ! Only allows viewing chat with accepted friends.
// TODO: Add pagination and search/filter for messages.
// * -------------------------------------------------------------
import { FriendModel } from '../../models/Friend.js';
import { MessageModel } from '../../models/Message.js';

/**
 * Get chat history with a friend
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function getChatHistory(req, res) {
  try {
    // ? Get friendId, page, limit, and current user
    const { friendId } = req.params;
    const { page = 1, limit = 50 } = req.query;
    const currentUserId = req.user.userId;

    // * Verify friendship exists
    const friendship = await FriendModel.findOne({
      $or: [
        { requester: currentUserId, recipient: friendId },
        { requester: friendId, recipient: currentUserId }
      ],
      status: 'accepted'
    });

    if (!friendship) {
      return res.status(403).json({ message: 'You can only view chat with friends' });
    }

    // * Get messages between the two users
    const messages = await MessageModel.find({
      $or: [
        { sender: currentUserId, recipient: friendId },
        { sender: friendId, recipient: currentUserId }
      ]
    })
    .populate('sender', 'username email')
    .sort({ createdAt: -1 })
    .limit(limit * 1)
    .skip((page - 1) * limit);

    // * Mark received messages as read
    await MessageModel.updateMany(
      { sender: friendId, recipient: currentUserId, isRead: false },
      { isRead: true }
    );

    // * Respond with messages and pagination info
    res.json({
      messages: messages.reverse(), // Return in chronological order
      page: parseInt(page),
      hasMore: messages.length === parseInt(limit)
    });
  } catch (error) {
    // ! Log and handle unexpected errors
    console.error('Get chat history error:', error);
    res.status(500).json({ message: 'Failed to get chat history' });
  }
}
