/*
 * Summary of file usage:
 * Express controller handling all room-related HTTP requests and business logic.
 * Manages room CRUD operations, member management, and room data retrieval with permissions.
 * 
 * Inputs of this file:
 * - HTTP requests for room operations (create, join, leave, delete, etc.)
 * - User authentication tokens and permission levels
 * - Room filtering and search parameters
 * 
 * Outputs of this file:
 * - JSON responses with room data and operation results
 * - HTTP status codes for success/error states
 * - Room member management and permission updates
 * - Graph data associated with specific rooms
 * 
 * 3 things that can be improved:
 * 1. Add input validation middleware and sanitization for all endpoints
 * 2. Implement caching layer for frequently accessed room data
 * 3. Add comprehensive audit logging for room operations and member changes
 * 
 * STATUS & REUSABILITY ASSESSMENT:
 * ✅ GOOD - Well-structured controller with room for enhancement
 * - Good separation of concerns and logical organization
 * - Comprehensive room operations coverage
 * - Proper error handling in most cases
 * - Could benefit from input validation middleware
 * - Some functions are quite long and could be refactored
 * - Generally follows Express controller best practices
 * - Easy to add new endpoints or modify existing ones
 */

import { RoomModel } from '../models/Room.js';
import { NodeModel } from '../models/Node.js';
import { EdgeModel } from '../models/Edge.js';
import { UserModel } from '../models/User.js';
import { ChallengeModel } from '../models/Challenge.js';
import mongoose from 'mongoose';
import { getSocketIO } from '../lib/socketIO.js';
import { DEBATE_SCHEMA, DEBATE_FORMATS } from '../lib/debate-schema.js';

/**
 * Utility functions for bulk data import/export
 */

// Validate debate structure against schema
function validateDebateStructure(data) {
  const errors = [];
  // Required sections
  if (!data.arguments) {
    errors.push('Missing required "arguments" section');
    return errors; // Can't continue without arguments
  }

  // Detect format type
  const isSimpleFormat = Array.isArray(data.arguments);
  const isComplexFormat = data.arguments && typeof data.arguments === 'object' && !Array.isArray(data.arguments);

  if (isSimpleFormat) {
    // Simple format validation: arguments is an array
    const argIds = new Set();
    data.arguments.forEach((arg, index) => {
      if (!arg.id) {
        errors.push(`Argument ${index}: missing required "id" field`);
      } else if (argIds.has(arg.id)) {
        errors.push(`Argument ${index}: duplicate ID "${arg.id}"`);
      } else {
        argIds.add(arg.id);
      }

      if (!arg.text) {
        errors.push(`Argument ${index}: missing required "text" field`);
      }

      if (arg.position && (typeof arg.position.x !== 'number' || typeof arg.position.y !== 'number')) {
        errors.push(`Argument ${index}: invalid position coordinates`);
      }
    });

    // Validate connections in simple format
    if (data.connections && Array.isArray(data.connections)) {
      data.connections.forEach((conn, index) => {
        if (!conn.from) {
          errors.push(`Connection ${index}: missing required "from" field`);
        }
        if (!conn.to) {
          errors.push(`Connection ${index}: missing required "to" field`);
        }
        if (!conn.type) {
          errors.push(`Connection ${index}: missing required "type" field`);
        }

        // Check if referenced arguments exist
        if (conn.from && !argIds.has(conn.from)) {
          errors.push(`Connection ${index}: references non-existent argument "${conn.from}"`);
        }
        if (conn.to && !argIds.has(conn.to)) {
          errors.push(`Connection ${index}: references non-existent argument "${conn.to}"`);
        }
      });
    }
  } else if (isComplexFormat) {
    // Complex format validation: arguments is an object with nodes/edges
    if (!data.arguments.nodes || !Array.isArray(data.arguments.nodes)) {
      errors.push('Missing or invalid "arguments.nodes" array');
    }

    if (data.arguments.edges && !Array.isArray(data.arguments.edges)) {
      errors.push('Invalid "arguments.edges" - must be an array');
    }

    // Validate nodes
    if (data.arguments.nodes) {
      const nodeIds = new Set();
      data.arguments.nodes.forEach((node, index) => {
        if (!node.id) {
          errors.push(`Node ${index}: missing required "id" field`);
        } else if (nodeIds.has(node.id)) {
          errors.push(`Node ${index}: duplicate ID "${node.id}"`);
        } else {
          nodeIds.add(node.id);
        }

        if (!node.label) {
          errors.push(`Node ${index}: missing required "label" field`);
        }

        if (node.position && (typeof node.position.x !== 'number' || typeof node.position.y !== 'number')) {
          errors.push(`Node ${index}: invalid position coordinates`);
        }
      });
    }

    // Validate edges
    if (data.arguments.edges) {
      data.arguments.edges.forEach((edge, index) => {
        if (!edge.source) {
          errors.push(`Edge ${index}: missing required "source" field`);
        }
        if (!edge.target) {
          errors.push(`Edge ${index}: missing required "target" field`);
        }
        if (!edge.relation) {
          errors.push(`Edge ${index}: missing required "relation" field`);
        }

        // Check if referenced nodes exist
        if (data.arguments.nodes) {
          const nodeIds = data.arguments.nodes.map(n => n.id);
          if (edge.source && !nodeIds.includes(edge.source)) {
            errors.push(`Edge ${index}: references non-existent source node "${edge.source}"`);
          }
          if (edge.target && !nodeIds.includes(edge.target)) {
            errors.push(`Edge ${index}: references non-existent target node "${edge.target}"`);
          }
        }
      });
    }
  } else {
    errors.push('Invalid arguments format - must be either an array (simple format) or object with nodes/edges (complex format)');
  }

  // Validate participants if present
  if (data.participants && !Array.isArray(data.participants)) {
    errors.push('Invalid "participants" - must be an array');
  }

  // Validate room settings if present
  if (data.roomSettings && typeof data.roomSettings !== 'object') {
    errors.push('Invalid "roomSettings" - must be an object');
  }

  return errors;
}

// Normalize debate data to complex format for consistent processing
function normalizeDebateData(data) {
  // If already in complex format, return as-is
  if (data.arguments && !Array.isArray(data.arguments)) {
    return data;
  }

  // Transform simple format to complex format
  const normalized = {
    metadata: {
      title: data.title || 'Imported Debate',
      description: data.description || '',
      format: 'custom',
      category: 'general',
      difficulty: 'intermediate',
      estimatedDuration: 60,
      language: 'en',
      tags: [],
      version: '1.0',
      importedAt: new Date().toISOString()
    },
    roomSettings: {
      name: data.title || 'Imported Debate',
      description: data.description || '',
      isPublic: data.settings?.isPublic ?? true,
      settings: data.settings || {}
    },
    participants: data.participants || [],
    arguments: {
      nodes: [],
      edges: []
    }
  };

  // Convert arguments array to nodes
  if (Array.isArray(data.arguments)) {
    normalized.arguments.nodes = data.arguments.map(arg => ({
      id: arg.id,
      label: arg.text,
      type: arg.type || 'claim',
      stance: arg.stance || 'neutral',
      strength: arg.strength || 5,
      position: arg.position || { x: 0, y: 0 },
      createdBy: 'imported',
      metadata: {
        argumentType: arg.type || 'claim',
        sources: arg.sources || [],
        tags: arg.tags || [],
        complexity: 5,
        evidenceQuality: 5,
        logicalSoundness: 5,
        style: { color: '#4CAF50', shape: 'circle', size: 'medium' },
        teachingNotes: '',
        custom: {}
      }
    }));
  }

  // Convert connections array to edges
  if (Array.isArray(data.connections)) {
    normalized.arguments.edges = data.connections.map((conn, index) => ({
      id: `edge_${index + 1}`,
      source: conn.from,
      target: conn.to,
      relation: conn.type === 'supports' ? 'supports' :
                conn.type === 'opposes' ? 'opposes' :
                conn.type === 'challenges' ? 'challenges' : 'supports',
      strength: conn.strength || 5,
      description: conn.description || '',
      createdBy: 'imported',
      metadata: {
        logicalType: 'deductive',
        evidenceStrength: conn.strength || 5,
        style: { color: '#4CAF50', thickness: 'medium', pattern: 'solid' },
        teachingNotes: '',
        custom: {}
      }
    }));
  }

  return normalized;
}

// Generate unique node ID if needed
function generateNodeId(existingIds = []) {
  let id;
  do {
    id = `node_${Date.now()}_${Math.random().toString(36).substr(2, 9)}`;
  } while (existingIds.includes(id));
  return id;
}

// Sanitize and validate room settings
function sanitizeRoomSettings(settings, currentSettings = {}) {
  const sanitized = { ...currentSettings };

  if (settings.name && typeof settings.name === 'string') {
    sanitized.name = settings.name.trim().substring(0, 100);
  }

  if (settings.description !== undefined) {
    sanitized.description = typeof settings.description === 'string'
      ? settings.description.trim().substring(0, 500)
      : '';
  }

  if (typeof settings.isPublic === 'boolean') {
    sanitized.isPublic = settings.isPublic;
  }

  if (typeof settings.maxMembers === 'number' && settings.maxMembers > 0) {
    sanitized.maxMembers = Math.min(Math.max(settings.maxMembers, 1), 200);
  }

  if (settings.settings && typeof settings.settings === 'object') {
    sanitized.settings = {
      ...currentSettings.settings,
      ...settings.settings,
      // Ensure boolean values
      isTurnBased: Boolean(settings.settings.isTurnBased),
      allowGuestEditing: Boolean(settings.settings.allowGuestEditing),
      requireReasonForConnections: Boolean(settings.settings.requireReasonForConnections),
      allowNodeDeletion: Boolean(settings.settings.allowNodeDeletion),
      allowEdgeDeletion: Boolean(settings.settings.allowEdgeDeletion),
      showMemberList: Boolean(settings.settings.showMemberList),
      showChallengeButton: Boolean(settings.settings.showChallengeButton),
      showExportButton: Boolean(settings.settings.showExportButton),
      // Ensure numeric values
      maxNodesPerUser: typeof settings.settings.maxNodesPerUser === 'number'
        ? Math.max(0, settings.settings.maxNodesPerUser)
        : 0
    };
  }

  return sanitized;
}

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
      NodeModel.find({ room: roomId }).populate('createdBy', 'email username'),
      EdgeModel.find({ room: roomId }).populate('createdBy', 'email username'),
      ChallengeModel.find({ room: roomId })
        .populate('challenger', 'email username')
        .populate('responder', 'email username')
    ]);

    // Add creator info to nodes (same format as socket utils)
    const nodesWithCreator = nodes.map(node => ({
      _id: node._id,
      id: node.id,
      label: node.label,
      metadata: node.metadata,
      x: node.x,
      y: node.y,
      room: node.room,
      createdBy: node.createdBy?.email,
      createdByUserId: node.createdBy?._id, // Add user ID for color assignment
      createdByUsername: node.createdBy?.username,
      createdAt: node.createdAt
    }));

    // Add creator info to edges
    const edgesWithCreator = edges.map(edge => ({
      ...edge.toObject(),
      createdBy: edge.createdBy?.email,
      createdByUserId: edge.createdBy?._id, // Add user ID for color assignment
      createdByUsername: edge.createdBy?.username
    }));

    res.json({
      nodes: nodesWithCreator,
      edges: edgesWithCreator,
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

/**
 * Start turn-based mode for a room
 */
export async function startTurnBasedMode(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    // Check if user is owner or admin
    const userRole = room.getMemberRole(userId);
    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Only room owners and admins can start turn-based mode' });
    }

    if (room.turnState.isActive) {
      return res.status(400).json({ message: 'Turn-based mode is already active' });
    }

    // Update settings and start turn-based mode
    room.settings.isTurnBased = true;
    const success = room.startTurnBasedMode();
    
    if (!success) {
      return res.status(400).json({ message: 'Failed to start turn-based mode - no members found' });
    }

    await room.save();

    // Emit socket event to all room members
    const io = getSocketIO();
    if (io) {
      io.to(roomId).emit('turn-mode-started', {
        currentPlayer: room.turnState.currentPlayer,
        turnNumber: room.turnState.turnNumber,
        startedAt: room.turnState.startedAt
      });
      console.log(`🎮 Turn-based mode started via API by user ${userId} in room ${room.name}`);
    }

    res.json({
      message: 'Turn-based mode started',
      turnState: {
        isActive: room.turnState.isActive,
        currentPlayer: room.turnState.currentPlayer,
        turnNumber: room.turnState.turnNumber,
        startedAt: room.turnState.startedAt
      }
    });
  } catch (error) {
    console.error('Start turn-based mode error:', error);
    res.status(500).json({ message: 'Failed to start turn-based mode' });
  }
}

/**
 * End current turn and move to next player
 */
export async function endTurn(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    if (!room.settings.isTurnBased || !room.turnState.isActive) {
      return res.status(400).json({ message: 'Turn-based mode is not active' });
    }

    // Check if it's the user's turn
    if (room.turnState.currentPlayer.toString() !== userId) {
      return res.status(403).json({ message: 'It is not your turn' });
    }

    const success = room.endTurn();
    if (!success) {
      return res.status(400).json({ message: 'Failed to end turn' });
    }

    await room.save();

    res.json({
      message: 'Turn ended successfully',
      turnState: {
        isActive: room.turnState.isActive,
        currentPlayer: room.turnState.currentPlayer,
        turnNumber: room.turnState.turnNumber,
        startedAt: room.turnState.startedAt,
        previousTurnActions: req.body.actions || []
      }
    });
  } catch (error) {
    console.error('End turn error:', error);
    res.status(500).json({ message: 'Failed to end turn' });
  }
}

/**
 * Stop turn-based mode for a room
 */
export async function stopTurnBasedMode(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    const room = await RoomModel.findById(roomId);
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    // Check if user is owner or admin
    const userRole = room.getMemberRole(userId);
    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Only room owners and admins can stop turn-based mode' });
    }

    // Update settings and stop turn-based mode
    room.settings.isTurnBased = false;
    room.turnState.isActive = false;
    room.turnState.currentPlayer = null;
    room.turnState.turnActions = [];

    await room.save();

    // Emit socket event to all room members
    const io = getSocketIO();
    if (io) {
      io.to(roomId).emit('turn-mode-stopped');
      console.log(`🛑 Turn-based mode stopped via API by user ${userId} in room ${room.name}`);
    }

    res.json({
      message: 'Turn-based mode stopped',
      turnState: {
        isActive: false,
        currentPlayer: null,
        turnNumber: room.turnState.turnNumber
      }
    });
  } catch (error) {
    console.error('Stop turn-based mode error:', error);
    res.status(500).json({ message: 'Failed to stop turn-based mode' });
  }
}

/**
 * Get current turn state for a room
 */
export async function getTurnState(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    const room = await RoomModel.findById(roomId).populate('turnState.currentPlayer', 'username email');
    if (!room) {
      return res.status(404).json({ message: 'Room not found' });
    }

    // Check if user is a member
    if (!room.hasMember(userId)) {
      return res.status(403).json({ message: 'Access denied - not a room member' });
    }

    res.json({
      turnState: {
        isActive: room.turnState.isActive,
        currentPlayer: room.turnState.currentPlayer,
        turnNumber: room.turnState.turnNumber,
        startedAt: room.turnState.startedAt,
        turnActions: room.turnState.turnActions,
        isTurnBased: room.settings.isTurnBased,
        isMyTurn: room.turnState.currentPlayer?.toString() === userId
      }
    });
  } catch (error) {
    console.error('Get turn state error:', error);
    res.status(500).json({ message: 'Failed to get turn state' });
  }
}

/**
 * Export room data as JSON for AI integration and backup
 */
export async function exportRoomToJson(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    // Get room and verify permissions
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

    // Only owners and admins can export
    const userRole = room.getMemberRole(userId);
    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Access denied - insufficient permissions' });
    }

    // Fetch all room data
    const [nodes, edges, challenges] = await Promise.all([
      NodeModel.find({ room: roomId }).populate('createdBy', 'email username'),
      EdgeModel.find({ room: roomId }).populate('createdBy', 'email username'),
      ChallengeModel.find({ room: roomId })
        .populate('challenger', 'email username')
        .populate('responder', 'email username')
    ]);

    // Build the complete debate structure following our schema
    const debateStructure = {
      metadata: {
        title: room.name,
        description: room.description || '',
        format: 'custom', // Could be enhanced to detect format
        category: 'general',
        difficulty: 'intermediate',
        estimatedDuration: 60,
        language: 'en',
        tags: [],
        createdBy: room.owner.username || room.owner.email,
        createdAt: room.createdAt.toISOString(),
        version: '1.0',
        exportedAt: new Date().toISOString(),
        exportedBy: req.user.email || req.user.username
      },

      roomSettings: {
        name: room.name,
        description: room.description || '',
        isPublic: room.isPublic,
        maxMembers: room.maxMembers,
        settings: room.settings
      },

      participants: room.members.map((member, index) => ({
        id: member.user._id.toString(),
        name: member.user.username || member.user.email,
        role: member.role,
        side: index % 2 === 0 ? 'affirmative' : 'negative', // Simple alternating assignment
        bio: '',
        isAI: false,
        speakingOrder: index + 1,
        timeAllocation: 10, // Default time allocation
        permissions: {
          canAddNodes: room.settings.allowMemberEdit || member.role !== 'member',
          canAddEdges: room.settings.allowMemberEdit || member.role !== 'member',
          canChallenge: room.settings.showChallengeButton,
          canModerate: member.role === 'owner' || member.role === 'admin'
        }
      })),

      arguments: {
        // Find potential resolution node (could be enhanced with better detection)
        resolution: nodes.length > 0 ? {
          id: nodes[0].id,
          text: nodes[0].label,
          type: 'resolution',
          position: { x: nodes[0].x || 0, y: nodes[0].y || 0 },
          metadata: {
            importance: 10,
            complexity: 5,
            sources: [],
            tags: []
          }
        } : null,

        nodes: nodes.map(node => ({
          id: node.id,
          label: node.label,
          type: node.metadata?.argumentType || 'claim',
          stance: node.metadata?.stance || 'neutral',
          strength: node.metadata?.strength || 5,
          position: { x: node.x || 0, y: node.y || 0 },
          createdBy: node.createdBy?._id?.toString() || 'unknown',
          metadata: {
            argumentType: node.metadata?.argumentType || 'claim',
            sources: node.metadata?.sources || [],
            tags: node.metadata?.tags || [],
            complexity: node.metadata?.complexity || 5,
            evidenceQuality: node.metadata?.evidenceQuality || 5,
            logicalSoundness: node.metadata?.logicalSoundness || 5,
            style: node.metadata?.style || { color: '#4CAF50', shape: 'circle', size: 'medium' },
            teachingNotes: node.metadata?.teachingNotes || '',
            custom: node.metadata?.custom || {}
          }
        })),

        edges: edges.map(edge => ({
          id: edge._id.toString(),
          source: edge.source,
          target: edge.target,
          relation: edge.relation || 'supports',
          strength: edge.metadata?.strength || 5,
          description: edge.metadata?.description || '',
          createdBy: edge.createdBy?._id?.toString() || 'unknown',
          metadata: {
            logicalType: edge.metadata?.logicalType || 'deductive',
            evidenceStrength: edge.metadata?.evidenceStrength || 5,
            style: edge.metadata?.style || { color: '#4CAF50', thickness: 'medium', pattern: 'solid' },
            teachingNotes: edge.metadata?.teachingNotes || '',
            custom: edge.metadata?.custom || {}
          }
        }))
      },

      structure: {
        phases: [
          {
            id: 'open_discussion',
            name: 'Open Discussion',
            description: 'Collaborative argument mapping',
            duration: 60,
            order: 1,
            participants: room.members.map(m => m.user._id.toString()),
            rules: {
              allowInterruptions: !room.settings.isTurnBased,
              allowQuestions: true,
              timeLimit: room.settings.isTurnBased ? 5 : 0,
              requiredElements: []
            }
          }
        ],

        speakingOrder: room.members.map((member, index) => ({
          participantId: member.user._id.toString(),
          phase: 'open_discussion',
          order: index + 1,
          timeLimit: room.settings.isTurnBased ? 5 : 0,
          requirements: []
        })),

        totalDuration: 60,
        preparationTime: 10,
        questionPeriods: []
      },

      evaluation: {
        criteria: [
          {
            id: 'logic',
            name: 'Logical Reasoning',
            description: 'Clarity and soundness of logical arguments',
            weight: 0.5,
            scale: { min: 1, max: 10, labels: { 1: 'Poor', 5: 'Average', 10: 'Excellent' } }
          },
          {
            id: 'evidence',
            name: 'Evidence Quality',
            description: 'Relevance and credibility of supporting evidence',
            weight: 0.5,
            scale: { min: 1, max: 10, labels: { 1: 'Poor', 5: 'Average', 10: 'Excellent' } }
          }
        ],

        judges: [],
        scoringMethod: 'collaborative',
        winCondition: 'consensus',

        rubric: {
          excellent: { min: 90, description: 'Outstanding argument with strong evidence and clear logic' },
          good: { min: 70, description: 'Solid argument with adequate support and reasoning' },
          fair: { min: 50, description: 'Basic argument with some logical gaps or weak evidence' },
          poor: { min: 0, description: 'Weak argument with significant flaws in logic or evidence' }
        }
      },

      educational: {
        learningObjectives: [],
        skillsFocused: ['critical thinking', 'collaboration', 'argument mapping'],
        gradeLevel: 'general',
        prerequisites: [],
        extensions: [],

        guidingQuestions: {
          preparation: [],
          during: [],
          reflection: []
        },

        resources: []
      },

      // Include challenges for completeness
      challenges: challenges.map(challenge => ({
        id: challenge._id.toString(),
        targetType: challenge.targetType,
        targetId: challenge.targetId,
        challenger: challenge.challenger?.username || challenge.challenger?.email || 'unknown',
        responder: challenge.responder?.username || challenge.responder?.email || 'unknown',
        reason: challenge.reason,
        status: challenge.status,
        createdAt: challenge.createdAt.toISOString(),
        respondedAt: challenge.respondedAt?.toISOString() || null
      })),

      // Export metadata
      exportInfo: {
        exportedAt: new Date().toISOString(),
        exportedBy: req.user.email || req.user.username,
        roomId: roomId,
        nodeCount: nodes.length,
        edgeCount: edges.length,
        memberCount: room.members.length,
        schemaVersion: '1.0'
      }
    };

    // Set appropriate headers for file download
    res.setHeader('Content-Type', 'application/json');
    res.setHeader('Content-Disposition', `attachment; filename="debate-${room.name.replace(/[^a-zA-Z0-9]/g, '_')}-${new Date().toISOString().split('T')[0]}.json"`);

    res.json(debateStructure);

  } catch (error) {
    console.error('Export room to JSON error:', error);
    res.status(500).json({ message: 'Failed to export room data' });
  }
}

/**
 * Import debate structure from JSON and populate room
 */
export async function importRoomFromJson(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;
    const debateData = req.body;



    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    // Validate that we have debate data
    if (!debateData || typeof debateData !== 'object') {
      return res.status(400).json({ message: 'Invalid debate data provided' });
    }

    // Get room and verify permissions
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

    // Only owners and admins can import
    const userRole = room.getMemberRole(userId);
    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Access denied - insufficient permissions' });
    }

    // Comprehensive validation of debate structure
    const validationErrors = validateDebateStructure(debateData);
    if (validationErrors.length > 0) {
      return res.status(400).json({
        message: 'Invalid debate structure',
        errors: validationErrors
      });
    }

    // Transform simple format to complex format if needed
    const normalizedData = normalizeDebateData(debateData);

    // Start a database transaction for data integrity
    const session = await mongoose.startSession();
    session.startTransaction();

    try {
      // Create backup before making changes (for undo functionality)
      const backupData = {
        timestamp: new Date().toISOString(),
        roomId: roomId,
        importedBy: userId,
        previousState: {
          nodes: await NodeModel.find({ room: roomId }).lean(),
          edges: await EdgeModel.find({ room: roomId }).lean(),
          challenges: await ChallengeModel.find({ room: roomId }).lean(),
          roomSettings: room.toObject()
        }
      };

      // Store backup in room metadata (simple approach - could be enhanced with dedicated backup collection)
      await RoomModel.findByIdAndUpdate(roomId, {
        $set: { 'metadata.lastImportBackup': backupData }
      }, { session });

      // Clear existing room data (optional - could be made configurable)
      const clearExisting = req.body.clearExisting !== false; // Default to true

      if (clearExisting) {
        await NodeModel.deleteMany({ room: roomId }, { session });
        await EdgeModel.deleteMany({ room: roomId }, { session });
        await ChallengeModel.deleteMany({ room: roomId }, { session });
      }

      // Update room settings if provided
      if (normalizedData.roomSettings) {
        const sanitizedSettings = sanitizeRoomSettings(normalizedData.roomSettings, room.toObject());

        if (Object.keys(sanitizedSettings).length > 0) {
          await RoomModel.findByIdAndUpdate(roomId, sanitizedSettings, { session });
        }
      }

      // Import nodes
      const nodeMap = new Map(); // Track old ID to new ID mapping
      const nodesToCreate = [];

      for (const nodeData of normalizedData.arguments.nodes) {
        // Find the user who should be the creator (fallback to current user)
        let creatorId = userId; // Default to current user

        if (nodeData.createdBy) {
          // Try to find matching user by ID first, then by username/email
          const creator = room.members.find(member =>
            member.user._id.toString() === nodeData.createdBy ||
            member.user.username === nodeData.createdBy ||
            member.user.email === nodeData.createdBy
          );
          if (creator) {
            creatorId = creator.user._id;
          }
        }

        const newNode = {
          id: nodeData.id || generateNodeId(nodesToCreate.map(n => n.id)),
          label: nodeData.label || 'Untitled Node',
          metadata: nodeData.metadata || {},
          x: nodeData.position?.x || Math.random() * 800 - 400,
          y: nodeData.position?.y || Math.random() * 600 - 300,
          room: roomId,
          createdBy: creatorId
        };

        nodesToCreate.push(newNode);
        nodeMap.set(nodeData.id, newNode.id);
      }

      // Create all nodes
      if (nodesToCreate.length > 0) {
        await NodeModel.insertMany(nodesToCreate, { session });
      }

      // Import edges
      const edgesToCreate = [];

      if (normalizedData.arguments.edges) {
        for (const edgeData of normalizedData.arguments.edges) {
          // Map old node IDs to new ones
          const sourceId = nodeMap.get(edgeData.source) || edgeData.source;
          const targetId = nodeMap.get(edgeData.target) || edgeData.target;

          // Skip edges that reference non-existent nodes
          const sourceExists = nodesToCreate.some(n => n.id === sourceId);
          const targetExists = nodesToCreate.some(n => n.id === targetId);

          if (!sourceExists || !targetExists) {
            console.warn(`Skipping edge ${edgeData.id} - source or target node not found`);
            continue;
          }

          // Find the user who should be the creator
          let creatorId = userId;

          if (edgeData.createdBy) {
            const creator = room.members.find(member =>
              member.user._id.toString() === edgeData.createdBy ||
              member.user.username === edgeData.createdBy ||
              member.user.email === edgeData.createdBy
            );
            if (creator) {
              creatorId = creator.user._id;
            }
          }

          const newEdge = {
            source: sourceId,
            target: targetId,
            relation: edgeData.relation || 'supports',
            metadata: edgeData.metadata || {},
            room: roomId,
            createdBy: creatorId
          };

          edgesToCreate.push(newEdge);
        }

        // Create all edges
        if (edgesToCreate.length > 0) {
          await EdgeModel.insertMany(edgesToCreate, { session });
        }
      }

      // Commit the transaction
      await session.commitTransaction();

      // Emit real-time update to all room members
      const io = getSocketIO();
      if (io) {
        // Fetch updated graph data
        const [updatedNodes, updatedEdges, challenges] = await Promise.all([
          NodeModel.find({ room: roomId }).populate('createdBy', 'email username'),
          EdgeModel.find({ room: roomId }).populate('createdBy', 'email username'),
          ChallengeModel.find({ room: roomId })
            .populate('challenger', 'email username')
            .populate('responder', 'email username')
        ]);

        const graphData = {
          nodes: updatedNodes.map(node => ({
            _id: node._id,
            id: node.id,
            label: node.label,
            metadata: node.metadata,
            x: node.x,
            y: node.y,
            room: node.room,
            createdBy: node.createdBy?.email,
            createdByUserId: node.createdBy?._id,
            createdByUsername: node.createdBy?.username,
            createdAt: node.createdAt
          })),
          edges: updatedEdges.map(edge => ({
            ...edge.toObject(),
            createdBy: edge.createdBy?.email,
            createdByUserId: edge.createdBy?._id,
            createdByUsername: edge.createdBy?.username
          })),
          challenges
        };

        io.to(roomId).emit('room-graph-updated', graphData);

        // Also emit a specific import notification
        io.to(roomId).emit('debate-structure-imported', {
          importedBy: req.user.email || req.user.username,
          timestamp: new Date().toISOString(),
          summary: {
            nodes: nodesToCreate.length,
            edges: edgesToCreate.length,
            roomUpdated: !!debateData.roomSettings,
            title: debateData.metadata?.title || 'Untitled Debate'
          },
          clearExisting
        });
      }

      res.json({
        message: 'Debate structure imported successfully',
        imported: {
          nodes: nodesToCreate.length,
          edges: edgesToCreate.length,
          roomUpdated: !!debateData.roomSettings
        },
        metadata: debateData.metadata || null
      });

    } catch (transactionError) {
      // Rollback transaction on error
      await session.abortTransaction();
      throw transactionError;
    } finally {
      session.endSession();
    }

  } catch (error) {
    console.error('Import room from JSON error:', error);
    res.status(500).json({
      message: 'Failed to import debate structure',
      error: error.message
    });
  }
}

/**
 * Undo last JSON import by restoring previous state
 */
export async function undoLastImport(req, res) {
  try {
    const { roomId } = req.params;
    const userId = req.user.userId;

    if (!mongoose.Types.ObjectId.isValid(roomId)) {
      return res.status(400).json({ message: 'Invalid room ID' });
    }

    // Get room and verify permissions
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

    // Only owners and admins can undo imports
    const userRole = room.getMemberRole(userId);
    if (userRole !== 'owner' && userRole !== 'admin') {
      return res.status(403).json({ message: 'Access denied - insufficient permissions' });
    }

    // Check if there's a backup to restore
    const backup = room.metadata?.lastImportBackup;
    if (!backup) {
      return res.status(404).json({ message: 'No import backup found to restore' });
    }

    // Check if backup is recent (within last 24 hours for safety)
    const backupTime = new Date(backup.timestamp);
    const now = new Date();
    const hoursSinceBackup = (now - backupTime) / (1000 * 60 * 60);

    if (hoursSinceBackup > 24) {
      return res.status(400).json({
        message: 'Backup is too old to restore safely (older than 24 hours)'
      });
    }

    // Start transaction for restoration
    const session = await mongoose.startSession();
    session.startTransaction();

    try {
      // Clear current data
      await NodeModel.deleteMany({ room: roomId }, { session });
      await EdgeModel.deleteMany({ room: roomId }, { session });
      await ChallengeModel.deleteMany({ room: roomId }, { session });

      // Restore nodes
      if (backup.previousState.nodes.length > 0) {
        const nodesToRestore = backup.previousState.nodes.map(node => ({
          ...node,
          _id: undefined // Let MongoDB generate new IDs
        }));
        await NodeModel.insertMany(nodesToRestore, { session });
      }

      // Restore edges
      if (backup.previousState.edges.length > 0) {
        const edgesToRestore = backup.previousState.edges.map(edge => ({
          ...edge,
          _id: undefined // Let MongoDB generate new IDs
        }));
        await EdgeModel.insertMany(edgesToRestore, { session });
      }

      // Restore challenges
      if (backup.previousState.challenges.length > 0) {
        const challengesToRestore = backup.previousState.challenges.map(challenge => ({
          ...challenge,
          _id: undefined // Let MongoDB generate new IDs
        }));
        await ChallengeModel.insertMany(challengesToRestore, { session });
      }

      // Restore room settings (excluding the backup metadata)
      const { metadata, ...roomSettingsToRestore } = backup.previousState.roomSettings;
      await RoomModel.findByIdAndUpdate(roomId, {
        ...roomSettingsToRestore,
        $unset: { 'metadata.lastImportBackup': 1 } // Remove the backup after successful restore
      }, { session });

      // Commit transaction
      await session.commitTransaction();

      // Emit real-time update
      const io = getSocketIO();
      if (io) {
        // Fetch restored graph data
        const [restoredNodes, restoredEdges, restoredChallenges] = await Promise.all([
          NodeModel.find({ room: roomId }).populate('createdBy', 'email username'),
          EdgeModel.find({ room: roomId }).populate('createdBy', 'email username'),
          ChallengeModel.find({ room: roomId })
            .populate('challenger', 'email username')
            .populate('responder', 'email username')
        ]);

        const graphData = {
          nodes: restoredNodes.map(node => ({
            _id: node._id,
            id: node.id,
            label: node.label,
            metadata: node.metadata,
            x: node.x,
            y: node.y,
            room: node.room,
            createdBy: node.createdBy?.email,
            createdByUserId: node.createdBy?._id,
            createdByUsername: node.createdBy?.username,
            createdAt: node.createdAt
          })),
          edges: restoredEdges.map(edge => ({
            ...edge.toObject(),
            createdBy: edge.createdBy?.email,
            createdByUserId: edge.createdBy?._id,
            createdByUsername: edge.createdBy?.username
          })),
          challenges: restoredChallenges
        };

        io.to(roomId).emit('room-graph-updated', graphData);

        // Emit undo notification
        io.to(roomId).emit('import-undone', {
          undoneBy: req.user.email || req.user.username,
          timestamp: new Date().toISOString(),
          backupTimestamp: backup.timestamp
        });
      }

      res.json({
        message: 'Import successfully undone',
        restored: {
          nodes: backup.previousState.nodes.length,
          edges: backup.previousState.edges.length,
          challenges: backup.previousState.challenges.length,
          backupTimestamp: backup.timestamp
        }
      });

    } catch (transactionError) {
      await session.abortTransaction();
      throw transactionError;
    } finally {
      session.endSession();
    }

  } catch (error) {
    console.error('Undo import error:', error);
    res.status(500).json({
      message: 'Failed to undo import',
      error: error.message
    });
  }
}
