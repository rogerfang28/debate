// * -------------------------------------------------------------
// * Friend Controller: declineFriendRequest.js
// ? Handles declining a pending friend request.
// ! Only the recipient of a pending request can decline it.
// TODO: Add notification to requester when declined.
// * -------------------------------------------------------------
import { FriendModel } from '../../models/Friend.js';

/**
 * Decline friend request
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function declineFriendRequest(req, res) {
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

    // * Mark the request as declined
    friendRequest.status = 'declined';
    await friendRequest.save();

    // * Respond with success
    res.json({ message: 'Friend request declined' });
  } catch (error) {
    // ! Log and handle unexpected errors
    console.error('Decline friend request error:', error);
    res.status(500).json({ message: 'Failed to decline friend request' });
  }
}
