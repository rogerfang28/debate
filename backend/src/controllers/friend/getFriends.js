import { FriendModel } from '../../models/Friend.js';

/**
 * Get friends list and pending requests
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function getFriends(req, res) {
  try {
    const currentUserId = req.user.userId;

    // Get accepted friends
    const acceptedFriends = await FriendModel.find({
      $or: [
        { requester: currentUserId, status: 'accepted' },
        { recipient: currentUserId, status: 'accepted' }
      ]
    }).populate('requester recipient', 'username email');

    // Get pending requests (both sent and received)
    const pendingRequests = await FriendModel.find({
      $or: [
        { requester: currentUserId, status: 'pending' },
        { recipient: currentUserId, status: 'pending' }
      ]
    }).populate('requester recipient', 'username email');

    // Format friends list
    const friends = acceptedFriends.map(friendship => {
      const friend = friendship.requester._id.toString() === currentUserId ? 
        friendship.recipient : friendship.requester;
      return {
        _id: friend._id,
        username: friend.username,
        email: friend.email,
        friendshipId: friendship._id
      };
    });

    // Format pending requests
    const sent = pendingRequests
      .filter(req => req.requester._id.toString() === currentUserId)
      .map(req => ({
        _id: req._id,
        user: {
          _id: req.recipient._id,
          username: req.recipient.username,
          email: req.recipient.email
        }
      }));

    const received = pendingRequests
      .filter(req => req.recipient._id.toString() === currentUserId)
      .map(req => ({
        _id: req._id,
        user: {
          _id: req.requester._id,
          username: req.requester.username,
          email: req.requester.email
        }
      }));

    res.json({
      friends,
      pending: { sent, received }
    });
  } catch (error) {
    console.error('Get friends error:', error);
    res.status(500).json({ message: 'Failed to get friends list' });
  }
}
