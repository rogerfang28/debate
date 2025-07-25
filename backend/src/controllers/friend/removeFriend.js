import { FriendModel } from '../../models/Friend.js';

/**
 * Remove friend
 * @param {import('express').Request} req
 * @param {import('express').Response} res
 * @returns {Promise<void>}
 * @author GitHub Copilot
 */
export async function removeFriend(req, res) {
  try {
    const { friendshipId } = req.params;
    const currentUserId = req.user.userId;

    const friendship = await FriendModel.findOne({
      _id: friendshipId,
      $or: [
        { requester: currentUserId },
        { recipient: currentUserId }
      ],
      status: 'accepted'
    });

    if (!friendship) {
      return res.status(404).json({ message: 'Friendship not found' });
    }

    await FriendModel.deleteOne({ _id: friendshipId });

    res.json({ message: 'Friend removed successfully' });
  } catch (error) {
    console.error('Remove friend error:', error);
    res.status(500).json({ message: 'Failed to remove friend' });
  }
}
