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

    friendRequest.status = 'declined';
    await friendRequest.save();

    res.json({ message: 'Friend request declined' });
  } catch (error) {
    console.error('Decline friend request error:', error);
    res.status(500).json({ message: 'Failed to decline friend request' });
  }
}
