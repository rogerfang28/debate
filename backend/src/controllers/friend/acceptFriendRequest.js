import { FriendModel } from '../../models/Friend.js';
import { UserModel } from '../../models/User.js';
import { emitToUser } from '../../lib/socketHandler.js';

/**
 * Accept friend request
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function acceptFriendRequest(req, res) {
  try {
    const { requestId } = req.params;
    const currentUserId = req.user.userId;

    const friendRequest = await FriendModel.findOne({
      _id: requestId,
      recipient: currentUserId,
      status: 'pending'
    });

    if (!friendRequest) {
      return res.status(404).json({ message: 'Friend request not found' });
    }

    friendRequest.status = 'accepted';
    await friendRequest.save();

    // Emit real-time notifications
    const accepter = await UserModel.findById(currentUserId).select('username email');
    
    // Emit to all components
    const eventData = {
      requestId: friendRequest._id,
      by: {
        _id: currentUserId,
        username: accepter.username,
        email: accepter.email
      }
    };
    
    emitToUser(friendRequest.requester.toString(), 'friend-request-accepted', eventData);
    emitToUser(friendRequest.requester.toString(), 'friend-request-accepted-chatfloat', eventData);
    emitToUser(friendRequest.requester.toString(), 'friend-request-accepted-modal', eventData);
    emitToUser(friendRequest.requester.toString(), 'friend-request-accepted-homepage', eventData);

    res.json({ message: 'Friend request accepted' });
  } catch (error) {
    console.error('Accept friend request error:', error);
    res.status(500).json({ message: 'Failed to accept friend request' });
  }
}
