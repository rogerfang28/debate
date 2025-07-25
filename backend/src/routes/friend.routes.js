import express from 'express';
//import { friendController } from '../controllers/friend.controller.js';
import * as friendController from '../controllers/friend/index.js';
import { authenticateToken } from '../middleware/auth.middleware.js';

const router = express.Router();

// All friend routes require authentication
router.use(authenticateToken);

// Search for users to add as friends
router.get('/search', friendController.searchUsers);

// Get friends list and pending requests
router.get('/', friendController.getFriends);

// Send friend request
router.post('/request/:userId', friendController.sendFriendRequest);

// Accept friend request
router.put('/request/:requestId/accept', friendController.acceptFriendRequest);

// Decline friend request
router.put('/request/:requestId/decline', friendController.declineFriendRequest);

// Remove friend
router.delete('/friendship/:friendshipId', friendController.removeFriend);

// Chat endpoints
router.get('/:friendId/messages', friendController.getChatHistory);
router.post('/:friendId/messages', friendController.sendMessage);

// Room invitation
router.post('/:friendId/invite/:roomId', friendController.sendRoomInvitation);

export default router;
