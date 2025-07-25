import { FriendModel } from '../../models/Friend.js';
import { UserModel } from '../../models/User.js';

/**
 * Search for users to add as friends
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function searchUsers(req, res) {
  try {
    const { query } = req.query;
    const currentUserId = req.user.userId;

    if (!query || query.length < 2) {
      return res.status(400).json({ 
        message: 'Search query must be at least 2 characters' 
      });
    }

    // Find users by username or email (case-insensitive)
    const users = await UserModel.find({
      _id: { $ne: currentUserId }, // Exclude current user
      $or: [
        { username: { $regex: query, $options: 'i' } },
        { email: { $regex: query, $options: 'i' } }
      ]
    }).select('username email').limit(10);

    // Get existing friend relationships to show status
    const userIds = users.map(user => user._id);
    const friendships = await FriendModel.find({
      $or: [
        { requester: currentUserId, recipient: { $in: userIds } },
        { requester: { $in: userIds }, recipient: currentUserId }
      ]
    });

    // Add friendship status to each user
    const usersWithStatus = users.map(user => {
      const friendship = friendships.find(f => 
        (f.requester.toString() === currentUserId && f.recipient.toString() === user._id.toString()) ||
        (f.recipient.toString() === currentUserId && f.requester.toString() === user._id.toString())
      );

      let status = 'none';
      if (friendship) {
        if (friendship.status === 'accepted') {
          status = 'friends';
        } else if (friendship.requester.toString() === currentUserId) {
          status = 'sent';
        } else {
          status = 'received';
        }
      }

      return {
        _id: user._id,
        username: user.username,
        email: user.email,
        friendshipStatus: status
      };
    });

    res.json({ users: usersWithStatus });
  } catch (error) {
    console.error('Search users error:', error);
    res.status(500).json({ message: 'Failed to search users' });
  }
}
