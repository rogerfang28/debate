import { RoomModel } from '../models/Room.js';
import { NodeModel } from '../models/Node.js';
import { EdgeModel } from '../models/Edge.js';
import { UserModel } from '../models/User.js';
import { ChallengeModel } from '../models/Challenge.js';
import mongoose from 'mongoose';

/**
 * Create a new room
 */
export async function createRoom(req, res) {
  try {
    const { name, description, isPublic = false, maxMembers = 50 } = req.body;
    const userId = req.user.userId; // Fixed: was req.user.id, should be req.user.userId

    if (!name || name.trim().length === 0) {
      return res.status(400).json({ message: 'Room name is required' });
    }

    const room = new RoomModel({
      name: name.trim(),
      description: description?.trim() || '',
      owner: userId,
      members: [{
        user: userId,
        role: 'owner',
        joinedAt: new Date()
      }],
      isPublic,
      maxMembers: Math.min(Math.max(maxMembers, 1), 200) // Clamp between 1-200
    });

    // Generate invite code if room is not public
    if (!isPublic) {
      room.generateInviteCode();
    }

    await room.save();
    await room.populate('owner', 'email');
    await room.populate('members.user', 'email');

    res.status(201).json({
      message: 'Room created successfully',
      room
    });
  } catch (error) {
    console.error('Create room error:', error);
    res.status(500).json({ message: 'Failed to create room' });
  }
}

/**
 * Get all rooms for the authenticated user
 */
export async function getRooms(req, res) {
  try {
    const userId = req.user.userId;

    const rooms = await RoomModel.find({
      'members.user': userId
    })
    .populate('owner', 'email username')
    .populate('members.user', 'email username')
    .sort({ updatedAt: -1 });

    // Add user's role and member count to each room
    const roomsWithMetadata = rooms.map(room => {
      const userRole = room.getMemberRole(userId);
      return {
        ...room.toObject(),
        userRole,
        memberCount: room.members.length
      };
    });

    res.json({ rooms: roomsWithMetadata });
  } catch (error) {
    console.error('Get rooms error:', error);
    res.status(500).json({ message: 'Failed to fetch rooms' });
  }
}

/**
 * Get specific room details
 */
export async function getRoom(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId)
      .populate('owner', 'email username')
      .populate('members.user', 'email username');

    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    // Check if user is a member
    if (!room.hasMember(userId)) {
      return res.status(403).json({ message: 'Access denied - not a room member' });
    }

    const userRole = room.getMemberRole(userId);
    
    res.json({
      room: {
        ...room.toObject(),
        userRole,
        memberCount: room.members.length
      }
    });
  } catch (error) {
    console.error('Get room error:', error);
    res.status(500).json({ message: 'Failed to fetch room' });
  }
}

/**
 * Update room details (owner/admin only)
 */
export async function updateRoom(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;
    const { name, description, isPublic, maxMembers, settings } = req.body;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    const userRole = room.getMemberRole(userId);
    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Access denied - insufficient permissions' });
    }

    // Update fields
    if (name !== undefined) room.name = name.trim();
    if (description !== undefined) room.description = description.trim();
    if (isPublic !== undefined) room.isPublic = isPublic;
    if (maxMembers !== undefined) {
      room.maxMembers = Math.min(Math.max(maxMembers, 1), 200);
    }
    if (settings !== undefined) {
      room.settings = { ...room.settings, ...settings };
    }

    await room.save();
    await room.populate('owner', 'email');
    await room.populate('members.user', 'email');

    res.json({
      message: 'Room updated successfully',
      room
    });
  } catch (error) {
    console.error('Update room error:', error);
    res.status(500).json({ message: 'Failed to update room' });
  }
}

/**
 * Delete room (owner only)
 */
export async function deleteRoom(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (room.owner.toString() !== userId) {
      return res.status(403).json({ message: 'Access denied - only room owner can delete' });
    }

    // Delete all associated data
    await Promise.all([
      NodeModel.deleteMany({ room: roomId }),
      EdgeModel.deleteMany({ room: roomId }),
      RoomModel.findByIdAndDelete(roomId)
    ]);

    res.json({ message: 'Room deleted successfully' });
  } catch (error) {
    console.error('Delete room error:', error);
    res.status(500).json({ message: 'Failed to delete room' });
  }
}

/**
 * Join room using invite code
 */
export async function joinRoomByCode(req, res) {
  try {
    const { inviteCode } = req.params;
    const userId = req.user.userId;

    const room = await RoomModel.findOne({ inviteCode })
      .populate('owner', 'email')
      .populate('members.user', 'email');

    if (!room) {
      return res.status(404).json({ message: 'Invalid invite code' });
    }

    // Check if already a member
    if (room.hasMember(userId)) {
      return res.status(400).json({ message: 'Already a member of this room' });
    }

    // Check room capacity
    if (room.members.length >= room.maxMembers) {
      return res.status(400).json({ message: 'Room is at maximum capacity' });
    }

    // Add user as member
    room.members.push({
      user: userId,
      role: 'member',
      joinedAt: new Date()
    });

    await room.save();
    await room.populate('members.user', 'email');

    res.json({
      message: 'Successfully joined room',
      room: {
        ...room.toObject(),
        userRole: 'member',
        memberCount: room.members.length
      }
    });
  } catch (error) {
    console.error('Join room error:', error);
    res.status(500).json({ message: 'Failed to join room' });
  }
}

/**
 * Invite user to room by email
 */
export async function inviteToRoom(req, res) {
  try {
    const { roomId } = req.params;
    const { email } = req.body;
    const userId = req.user.userId;

    if (!email) {
      return res.status(400).json({ message: 'Email is required' });
    }

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    const userRole = room.getMemberRole(userId);
    if (!userRole) {
      return res.status(403).json({ message: 'Access denied - not a room member' });
    }

    // Check permissions
    if (userRole === 'member' && !room.settings.allowMemberInvites) {
      return res.status(403).json({ message: 'Access denied - members cannot send invites' });
    }

    // Find user to invite
    const targetUser = await UserModel.findOne({ email });
    if (!targetUser) {
      return res.status(404).json({ message: 'User not found' });
    }

    // Check if already a member
    if (room.hasMember(targetUser._id)) {
      return res.status(400).json({ message: 'User is already a member' });
    }

    // Check room capacity
    if (room.members.length >= room.maxMembers) {
      return res.status(400).json({ message: 'Room is at maximum capacity' });
    }

    // Generate invite code if none exists
    if (!room.inviteCode) {
      room.generateInviteCode();
      await room.save();
    }

    // In a real app, you'd send an email here
    // For now, just return the invite code
    res.json({
      message: 'Invite ready',
      inviteCode: room.inviteCode,
      inviteUrl: `/join/${room.inviteCode}`
    });
  } catch (error) {
    console.error('Invite to room error:', error);
    res.status(500).json({ message: 'Failed to send invite' });
  }
}

/**
 * Remove member from room
 */
export async function removeMember(req, res) {
  try {
    const { roomId, userId: targetUserId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId) || !mongoose.Types.ObjectId.isValid(targetUserId)) {
      return res.status(400).json({ message: 'Invalid room or user ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    const userRole = room.getMemberRole(userId);
    const targetRole = room.getMemberRole(targetUserId);

    // Permission checks
    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Access denied - insufficient permissions' });
    }

    if (targetUserId === room.owner.toString()) {
      return res.status(400).json({ message: 'Cannot remove room owner' });
    }

    if (userRole === 'admin' && targetRole === 'admin') {
      return res.status(403).json({ message: 'Admins cannot remove other admins' });
    }

    // Remove member
    room.members = room.members.filter(
      member => member.user.toString() !== targetUserId
    );

    await room.save();

    res.json({ message: 'Member removed successfully' });
  } catch (error) {
    console.error('Remove member error:', error);
    res.status(500).json({ message: 'Failed to remove member' });
  }
}

/**
 * Update member role (owner only)
 */
export async function updateMemberRole(req, res) {
  try {
    const { roomId, userId: targetUserId } = req.params;
    const { role } = req.body;
    const userId = req.user.userId;

    if (!['member', 'admin'].includes(role)) {
      return res.status(400).json({ message: 'Invalid role' });
    }

    if (!mongoose.Types.ObjectId.isValid(roomId) || !mongoose.Types.ObjectId.isValid(targetUserId)) {
      return res.status(400).json({ message: 'Invalid room or user ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (room.owner.toString() !== userId) {
      return res.status(403).json({ message: 'Access denied - only owner can change roles' });
    }

    if (targetUserId === room.owner.toString()) {
      return res.status(400).json({ message: 'Cannot change owner role' });
    }

    // Update member role
    const member = room.members.find(m => m.user.toString() === targetUserId);
    if (!member) {
      return res.status(404).json({ message: 'Member not found' });
    }

    member.role = role;
    await room.save();

    res.json({ message: 'Member role updated successfully' });
  } catch (error) {
    console.error('Update member role error:', error);
    res.status(500).json({ message: 'Failed to update member role' });
  }
}

/**
 * Leave room
 */
export async function leaveRoom(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (room.owner.toString() === userId) {
      return res.status(400).json({ message: 'Room owner cannot leave. Transfer ownership or delete room.' });
    }

    if (!room.hasMember(userId)) {
      return res.status(400).json({ message: 'Not a member of this room' });
    }

    // Remove member
    room.members = room.members.filter(
      member => member.user.toString() !== userId
    );

    await room.save();

    res.json({ message: 'Left room successfully' });
  } catch (error) {
    console.error('Leave room error:', error);
    res.status(500).json({ message: 'Failed to leave room' });
  }
}

/**
 * Generate new invite code
 */
export async function generateInviteCode(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    const userRole = room.getMemberRole(userId);
    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Access denied - insufficient permissions' });
    }

    const newCode = room.generateInviteCode();
    await room.save();

    res.json({
      message: 'New invite code generated',
      inviteCode: newCode,
      inviteUrl: `/join/${newCode}`
    });
  } catch (error) {
    console.error('Generate invite code error:', error);
    res.status(500).json({ message: 'Failed to generate invite code' });
  }
}

/**
 * Get room's graph data (nodes and edges)
 */
export async function getRoomGraph(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (!room.hasMember(userId)) {
      return res.status(403).json({ message: 'Access denied - not a room member' });
    }

    // Fetch graph data for this room
    const [nodes, edges, challenges] = await Promise.all([
      NodeModel.find({ room: roomId }).populate('createdBy', 'email'),
      EdgeModel.find({ room: roomId }).populate('createdBy', 'email'),
      ChallengeModel.find({ room: roomId })
        .populate('challenger', 'email username')
        .populate('responder', 'email username')
    ]);

    res.json({
      nodes,
      edges,
      challenges,
      room: {
        id: room._id,
        name: room.name,
        description: room.description
      }
    });
  } catch (error) {
    console.error('Get room graph error:', error);
    res.status(500).json({ message: 'Failed to fetch room graph' });
  }
}

/**
 * Get room members with usernames for challenge dropdown
 */
export async function getRoomMembers(req, res) {
  console.log('🎯 getRoomMembers called!', { roomId: req.params.roomId, userId: req.user?.userId });
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (!room.hasMember(userId)) {
      return res.status(403).json({ message: 'Access denied - not a room member' });
    }

    // Get all room members with their usernames and emails
    await room.populate('members.user', 'email username');

    console.log('🔍 Debug room members:', {
      roomId,
      currentUserId: userId,
      totalMembers: room.members.length,
      membersData: room.members.map(m => ({
        id: m.user._id.toString(),
        email: m.user.email,
        username: m.user.username,
        isCurrentUser: m.user._id.toString() === userId
      }))
    });

    const members = room.members
      .filter(member => {
        const isCurrentUser = member.user._id.toString() === userId;
        console.log(`👤 Checking member ${member.user.email}: isCurrentUser=${isCurrentUser}`);
        // For now, include current user for testing - we can exclude later
        return true; // Include all members for testing
      })
      .map(member => ({
        id: member.user._id,
        email: member.user.email,
        username: member.user.username || member.user.email.split('@')[0], // Fallback to email username
        displayName: member.user.username || member.user.email,
        isCurrentUser: member.user._id.toString() === userId
      }));

    console.log('✅ Filtered members for dropdown:', members);

    res.json({ members });
  } catch (error) {
    console.error('Get room members error:', error);
    res.status(500).json({ message: 'Failed to fetch room members' });
  }
}

/**
 * Get public rooms for browsing (no authentication required)
 */
export async function getPublicRooms(req, res) {
  try {
    const { page = 1, limit = 20, search = '' } = req.query;
    const skip = (page - 1) * limit;

    // Build search filter
    const filter = { isPublic: true };
    if (search.trim()) {
      filter.$or = [
        { name: { $regex: search, $options: 'i' } },
        { description: { $regex: search, $options: 'i' } }
      ];
    }

    // Get public rooms with basic info
    const rooms = await RoomModel.find(filter)
      .select('name description createdAt members')
      .populate('owner', 'email username')
      .sort({ createdAt: -1 })
      .skip(skip)
      .limit(parseInt(limit));

    // Get total count for pagination
    const total = await RoomModel.countDocuments(filter);

    // Add member count to each room
    const roomsWithCounts = await Promise.all(
      rooms.map(async (room) => {
        const memberCount = room.members?.length || 0;
        return {
          _id: room._id,
          name: room.name,
          description: room.description,
          owner: room.owner,
          createdAt: room.createdAt,
          memberCount
        };
      })
    );

    res.json({
      rooms: roomsWithCounts,
      pagination: {
        page: parseInt(page),
        limit: parseInt(limit),
        total,
        pages: Math.ceil(total / limit)
      }
    });
  } catch (error) {
    console.error('Get public rooms error:', error);
    res.status(500).json({ message: 'Failed to fetch public rooms' });
  }
}

/**
 * Get public room graph data (no authentication required)
 */
export async function getPublicRoomGraph(req, res) {
  try {
    const { roomId } = req.params;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    // Verify room exists and is public
    const room = await RoomModel.findById(roomId);
    console.log('🔍 Room found:', room ? 'YES' : 'NO');
    console.log('🌍 Is public:', room?.isPublic);
    console.log('📋 Room details:', {
      id: room?._id,
      name: room?.name,
      isPublic: room?.isPublic,
      memberCount: room?.members?.length || 0
    });
    
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (!room.isPublic) {
      return res.status(403).json({ message: 'This room is not public' });
    }

    // Get nodes and edges for the room
    const [nodes, edges, challenges] = await Promise.all([
      NodeModel.find({ room: roomId }),
      EdgeModel.find({ room: roomId }),
      ChallengeModel.find({ room: roomId, status: 'pending' })
    ]);

    console.log('📊 Database query results:');
    console.log('  - Nodes found:', nodes.length);
    console.log('  - Edges found:', edges.length);
    console.log('  - Challenges found:', challenges.length);
    
    if (nodes.length > 0) {
      console.log('📝 Node details:', nodes.map(n => ({ id: n.id, label: n.label })));
    }
    if (edges.length > 0) {
      console.log('🔗 Edge details:', edges.map(e => ({ source: e.source, target: e.target })));
    }

    res.json({
      room: {
        _id: room._id,
        name: room.name,
        description: room.description,
        createdAt: room.createdAt
      },
      nodes,
      edges,
      challenges
    });
  } catch (error) {
    console.error('Get public room graph error:', error);
    res.status(500).json({ message: 'Failed to fetch public room graph' });
  }
}
