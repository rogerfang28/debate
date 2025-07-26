// * -------------------------------------------------------------
// * Friend Controller: acceptFriendRequest.js
// ? Handles accepting a pending friend request and notifies the requester in real-time.
// ! Only the recipient of a pending request can accept it.
// TODO: Consider rate-limiting or additional validation for abuse prevention.
// * -------------------------------------------------------------
import { FriendModel } from '../../models/Friend.js';
import { UserModel } from '../../models/User.js';
import { emitToUser } from '../../socket/socketHandler.js';

/**
 * Accept friend request
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function acceptFriendRequest(req, res) {
  try {
    // ? Extract requestId from route params and get current user
    const { requestId } = req.params;
    const currentUserId = req.user.userId;

    // * Find the pending friend request for this user
    const friendRequest = await FriendModel.findOne({
      _id: requestId,
      recipient: currentUserId,
      status: 'pending'
    });

    // ! If not found, return 404 (request may be invalid or already handled)
    if (!friendRequest) {
      return res.status(404).json({ message: 'Friend request not found' });
    }

    // * Mark the request as accepted
    friendRequest.status = 'accepted';
    await friendRequest.save();

    // * Notify the requester in real-time via socket events
    const accepter = await UserModel.findById(currentUserId).select('username email');
    
    // ? Prepare event data for all relevant UI components
    const eventData = {
      requestId: friendRequest._id,
      by: {
        _id: currentUserId,
        username: accepter.username,
        email: accepter.email
      }
    };
    
    // * Emit to all relevant frontend listeners
    emitToUser(friendRequest.requester.toString(), 'friend-request-accepted', eventData); // Main event
    emitToUser(friendRequest.requester.toString(), 'friend-request-accepted-chatfloat', eventData); // ChatFloat UI
    emitToUser(friendRequest.requester.toString(), 'friend-request-accepted-modal', eventData); // Modal UI
    emitToUser(friendRequest.requester.toString(), 'friend-request-accepted-homepage', eventData); // Homepage UI

    // * Respond with success
    res.json({ message: 'Friend request accepted' });
  } catch (error) {
    // ! Log and handle unexpected errors
    console.error('Accept friend request error:', error);
    res.status(500).json({ message: 'Failed to accept friend request' });
  }
}
