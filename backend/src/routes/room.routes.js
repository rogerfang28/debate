import express from 'express';
import { 
  createRoom, 
  getRooms, 
  getRoom, 
  updateRoom, 
  deleteRoom,
  joinRoomByCode,
  inviteToRoom,
  removeMember,
  updateMemberRole,
  leaveRoom,
  generateInviteCode,
  getRoomGraph,
  getRoomMembers,
  getPublicRooms,
  getPublicRoomGraph
} from '../controllers/room/index.js';
import { authenticateToken } from '../middleware/auth.middleware.js';

const router = express.Router();

// Public routes (no authentication required)
router.get('/public', getPublicRooms);                    // Get public rooms for browsing
router.get('/public/:roomId/graph', getPublicRoomGraph);  // Get public room graph data

// All other room routes require authentication
router.use(authenticateToken);

// Room CRUD operations
router.post('/', createRoom);                    // Create new room
router.get('/', getRooms);                       // Get user's rooms
router.get('/:roomId', getRoom);                 // Get specific room details
router.put('/:roomId', updateRoom);              // Update room (owner/admin only)
router.delete('/:roomId', deleteRoom);           // Delete room (owner only)

// Room membership operations
router.post('/join/:inviteCode', joinRoomByCode); // Join room via invite code
router.post('/:roomId/invite', inviteToRoom);     // Invite user to room
router.delete('/:roomId/members/:userId', removeMember); // Remove member (admin+)
router.put('/:roomId/members/:userId/role', updateMemberRole); // Update member role (owner only)
router.post('/:roomId/leave', leaveRoom);         // Leave room

// Utility operations
router.post('/:roomId/invite-code', generateInviteCode); // Generate new invite code
router.get('/:roomId/graph', getRoomGraph);      // Get room's graph data
router.get('/:roomId/members', getRoomMembers);  // Get room members for challenges

// Test endpoint to verify routes are working
router.get('/:roomId/test', (req, res) => {
  res.json({ message: 'Test endpoint working', roomId: req.params.roomId });
});

export default router;
