// * -------------------------------------------------------------
// * Friend Controller: removeFriend.js
// ? Handles removing a friend from the user's friend list.
// ! Only allows removal of accepted friendships.
// TODO: Add notification to the removed friend.
// * -------------------------------------------------------------
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
    // ? Get friendshipId and current user
    const { friendshipId } = req.params;
    const currentUserId = req.user.userId;

    // * Find the accepted friendship
    const friendship = await FriendModel.findOne({
      _id: friendshipId,
      $or: [
        { requester: currentUserId },
        { recipient: currentUserId }
      ],
      status: 'accepted'
    });

    // ! If not found, return 404
    if (!friendship) {
      return res.status(404).json({ message: 'Friendship not found' });
    }

    // * Remove the friendship
    await FriendModel.deleteOne({ _id: friendshipId });

    // * Respond with success
    res.json({ message: 'Friend removed successfully' });
  } catch (error) {
    // ! Log and handle unexpected errors
    console.error('Remove friend error:', error);
    res.status(500).json({ message: 'Failed to remove friend' });
  }
}
