import { FriendModel } from '../../models/Friend.js';
import { UserModel } from '../../models/User.js';
import { emitToUser } from '../../lib/socketHandler.js';

/**
 * Send friend request
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function sendFriendRequest(req, res) {
  try {
    const { userId } = req.params;
    const currentUserId = req.user.userId;

    if (userId === currentUserId) {
      return res.status(400).json({ message: 'Cannot send friend request to yourself' });
    }

    // Check if user exists
    const targetUser = await UserModel.findById(userId);
    if (!targetUser) {
      return res.status(404).json({ message: 'User not found' });
    }

    // Check if friendship already exists
    const existingFriendship = await FriendModel.findOne({
      $or: [
        { requester: currentUserId, recipient: userId },
        { requester: userId, recipient: currentUserId }
      ]
    });

    if (existingFriendship) {
      return res.status(400).json({ 
        message: 'Friend request already exists or users are already friends' 
      });
    }

    // Create friend request
    const friendRequest = new FriendModel({
      requester: currentUserId,
      recipient: userId
    });

    await friendRequest.save();

    // Emit real-time notification to recipient
    const requester = await UserModel.findById(currentUserId).select('username email');
    
    // Emit to all components
    const eventData = {
      requestId: friendRequest._id,
      from: {
        _id: currentUserId,
        username: requester.username,
        email: requester.email
      }
    };
    
    emitToUser(userId, 'friend-request-received', eventData);
    emitToUser(userId, 'friend-request-received-chatfloat', eventData);
    emitToUser(userId, 'friend-request-received-modal', eventData);
    emitToUser(userId, 'friend-request-received-homepage', eventData);

    res.status(201).json({ 
      message: 'Friend request sent successfully',
      requestId: friendRequest._id
    });
  } catch (error) {
    console.error('Send friend request error:', error);
    res.status(500).json({ message: 'Failed to send friend request' });
  }
}
