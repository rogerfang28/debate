// * -------------------------------------------------------------
// * Friend Controller: sendFriendRequest.js
// ? Handles sending a friend request and notifies the recipient in real-time.
// ! Prevents duplicate requests and self-requests.
// TODO: Add notification batching or request expiration.
// * -------------------------------------------------------------
import { FriendModel } from '../../models/Friend.js';
import { UserModel } from '../../models/User.js';
import { emitToUser } from '../../socket/socketHandler.js';

/**
 * Send friend request
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function sendFriendRequest(req, res) {
  try {
    // ? Get target userId and current user
    const { userId } = req.params;
    const currentUserId = req.user.userId;

    // ! Prevent sending request to self
    if (userId === currentUserId) {
      return res.status(400).json({ message: 'Cannot send friend request to yourself' });
    }

    // * Check if user exists
    const targetUser = await UserModel.findById(userId);
    if (!targetUser) {
      return res.status(404).json({ message: 'User not found' });
    }

    // * Check for existing friendship or request
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

    // * Create friend request
    const friendRequest = new FriendModel({
      requester: currentUserId,
      recipient: userId
    });

    await friendRequest.save();

    // * Notify recipient in real-time
    const requester = await UserModel.findById(currentUserId).select('username email');
    
    // ? Prepare event data for all relevant UI components
    const eventData = {
      requestId: friendRequest._id,
      from: {
        _id: currentUserId,
        username: requester.username,
        email: requester.email
      }
    };
    
    // * Emit to all relevant frontend listeners
    emitToUser(userId, 'friend-request-received', eventData);
    emitToUser(userId, 'friend-request-received-chatfloat', eventData);
    emitToUser(userId, 'friend-request-received-modal', eventData);
    emitToUser(userId, 'friend-request-received-homepage', eventData);

    // * Respond with success
    res.status(201).json({ 
      message: 'Friend request sent successfully',
      requestId: friendRequest._id
    });
  } catch (error) {
    // ! Log and handle unexpected errors
    console.error('Send friend request error:', error);
    res.status(500).json({ message: 'Failed to send friend request' });
  }
}
