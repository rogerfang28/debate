import {
  handleAddNode,
  handleDeleteNode,
  handleAddEdge,
  handleDeleteEdge,
  handleUpdateNodePosition
} from '../functions/socketFunctions/index.js';
import { handleChallenge } from '../functions/socketFunctions/addChallenge.js';
import { handleChallengeResponse } from '../functions/socketFunctions/respondChallenge.js';
import jwt from 'jsonwebtoken';
import { RoomModel } from '../models/Room.js';
import { RoomMessageModel } from '../models/RoomMessage.js';

const JWT_SECRET = process.env.JWT_SECRET;

// Track user socket connections for friend/chat notifications
const userSockets = new Map();

// Helper function to emit to specific user
export function emitToUser(userId, event, data) {
  console.log(`🎯 Emitting "${event}" to user ${userId}:`, data);
  const userSocketIds = userSockets.get(userId);
  
  if (userSocketIds && userSocketIds.size > 0) {
    console.log(`📡 Found ${userSocketIds.size} socket(s) for user ${userId}`);
    userSocketIds.forEach(socketId => {
      const socket = global.io?.sockets.sockets.get(socketId);
      if (socket) {
        console.log(`✅ Emitting to socket ${socketId}`);
        socket.emit(event, data);
      } else {
        console.log(`❌ Socket ${socketId} not found, removing from tracking`);
        userSocketIds.delete(socketId);
      }
    });
  } else {
    console.log(`❌ No sockets found for user ${userId}`);
  }
}

export function registerSocketHandlers(io) {
  // Store io globally for emitToUser function
  global.io = io;

  // Socket authentication middleware
  io.use(async (socket, next) => {
    try {
      const token = socket.handshake.auth.token || socket.handshake.headers.authorization?.split(' ')[1];
      
      if (!token) {
        return next(new Error('Authentication token required'));
      }

      const decoded = jwt.verify(token, JWT_SECRET);
      socket.userId = decoded.userId; // Fixed: was decoded.id, should be decoded.userId
      socket.userEmail = decoded.email;
      
      console.log('🔐 Socket authenticated:', socket.userId, socket.userEmail);
      next();
    } catch (error) {
      console.error('❌ Socket authentication failed:', error.message);
      next(new Error('Authentication failed'));
    }
  });

  io.on('connection', socket => {
    console.log('⚡ Client connected:', socket.id, 'User:', socket.userEmail, 'UserID:', socket.userId);

    // Track user connections for friend/chat notifications
    if (!userSockets.has(socket.userId)) {
      userSockets.set(socket.userId, new Set());
    }
    userSockets.get(socket.userId).add(socket.id);
    console.log(`👤 User ${socket.userId} now has ${userSockets.get(socket.userId).size} socket(s)`);

    socket.onAny((event, ...args) => {
      console.log('📩 Received event:', event, 'from:', socket.userEmail, args);
    });

    // Room management
    socket.on('join-room', async (roomId) => {
      try {
        // Verify user is member of the room
        const room = await RoomModel.findById(roomId);
        if (!room) {
          socket.emit('error', { message: 'Room not found' });
          return;
        }

        if (!room.hasMember(socket.userId)) {
          socket.emit('error', { message: 'Access denied - not a room member' });
          return;
        }

        // Join the room
        socket.join(roomId);
        socket.currentRoom = roomId;
        socket.currentRoomName = room.name;
        
        console.log(`👥 User ${socket.userEmail} joined room: ${room.name} (${roomId})`);
        
        // Get current online users in this room
        const roomSockets = await io.in(roomId).fetchSockets();
        const onlineUsers = roomSockets.map(s => ({
          userId: s.userId,
          userEmail: s.userEmail,
          socketId: s.id
        }));

        // Load recent chat messages for this room (last 50 messages)
        const recentMessages = await RoomMessageModel.find({ room: roomId })
          .populate('sender', 'username email')
          .sort({ createdAt: -1 })
          .limit(50)
          .lean();

        // Reverse to show oldest first
        const messagesForClient = recentMessages.reverse().map(msg => ({
          roomId: roomId,
          userId: msg.sender._id,
          username: msg.sender.username || msg.sender.email?.split('@')[0] || 'Anonymous',
          message: msg.content,
          timestamp: msg.createdAt.toISOString(),
          messageId: msg._id
        }));
        
        // Notify others in the room about new user
        socket.to(roomId).emit('user-joined-room', {
          userId: socket.userId,
          userEmail: socket.userEmail,
          roomId
        });
        
        // Send current online users and chat history to the joining user
        socket.emit('joined-room', { 
          roomId, 
          roomName: room.name,
          onlineUsers: onlineUsers,
          messages: messagesForClient
        });
      } catch (error) {
        console.error('❌ Join room error:', error);
        socket.emit('error', { message: 'Failed to join room' });
      }
    });

    socket.on('leave-room', () => {
      if (socket.currentRoom) {
        socket.leave(socket.currentRoom);
        socket.to(socket.currentRoom).emit('user-left-room', {
          userId: socket.userId,
          userEmail: socket.userEmail,
          roomId: socket.currentRoom
        });
        console.log(`👋 User ${socket.userEmail} left room: ${socket.currentRoomName}`);
        socket.currentRoom = null;
        socket.currentRoomName = null;
      }
    });

    // Graph operations (room-scoped)
    socket.on('add-node', node => {
      if (!socket.currentRoom) {
        socket.emit('error', { message: 'Must join a room first' });
        return;
      }
      handleAddNode(io, socket, { ...node, room: socket.currentRoom });
    });

    socket.on('delete-node', payload => {
      if (!socket.currentRoom) {
        socket.emit('error', { message: 'Must join a room first' });
        return;
      }
      handleDeleteNode(io, socket, { ...payload, room: socket.currentRoom });
    });

    socket.on('add-edge', edge => {
      if (!socket.currentRoom) {
        socket.emit('error', { message: 'Must join a room first' });
        return;
      }
      handleAddEdge(io, socket, { ...edge, room: socket.currentRoom });
    });

    socket.on('delete-edge', criteria => {
      if (!socket.currentRoom) {
        socket.emit('error', { message: 'Must join a room first' });
        return;
      }
      handleDeleteEdge(io, socket, { ...criteria, room: socket.currentRoom });
    });

    socket.on('update-node-position', payload => {
      const roomId = payload.room || socket.currentRoom;
      if (!roomId) {
        socket.emit('error', { message: 'Must join a room first or provide room ID' });
        return;
      }
      handleUpdateNodePosition(io, socket, { ...payload, room: roomId });
    });

    socket.on('challenge', payload => {
      console.log('📡 Challenge event received from:', socket.userEmail, 'payload:', payload);
      console.log('📍 Current room:', socket.currentRoom);
      
      if (!socket.currentRoom) {
        console.error('❌ No current room for challenge');
        socket.emit('error', { message: 'Must join a room first' });
        return;
      }
      handleChallenge(io, socket, { ...payload, room: socket.currentRoom });
    });

    socket.on('respond-to-challenge', payload => {
      if (!socket.currentRoom) {
        socket.emit('error', { message: 'Must join a room first' });
        return;
      }
      handleChallengeResponse(io, socket, { ...payload, room: socket.currentRoom });
    });

    // Room chat messages
    socket.on('room-message', async payload => {
      if (!socket.currentRoom) {
        socket.emit('error', { message: 'Must join a room first' });
        return;
      }
      
      console.log('💬 Room message from:', socket.userEmail, 'message:', payload.message);
      
      try {
        // Save message to database
        const newMessage = new RoomMessageModel({
          room: socket.currentRoom,
          sender: socket.userId,
          content: payload.message,
          messageType: 'text'
        });

        const savedMessage = await newMessage.save();
        
        // Populate sender info
        await savedMessage.populate('sender', 'username email');
        
        const messageData = {
          roomId: socket.currentRoom,
          userId: socket.userId,
          username: savedMessage.sender.username || savedMessage.sender.email?.split('@')[0] || 'Anonymous',
          message: payload.message,
          timestamp: savedMessage.createdAt.toISOString(),
          messageId: savedMessage._id
        };

        // Broadcast message to all users in the room (including sender)
        io.to(socket.currentRoom).emit('room-message', messageData);
        
      } catch (error) {
        console.error('❌ Error saving room message:', error);
        socket.emit('error', { message: 'Failed to send message' });
      }
    });

    socket.on('disconnect', (reason) => {
      console.log('💀 Client disconnected:', socket.id, 'User:', socket.userEmail, 'UserID:', socket.userId, 'Reason:', reason);
      
      if (socket.currentRoom) {
        socket.to(socket.currentRoom).emit('user-left-room', {
          userId: socket.userId,
          userEmail: socket.userEmail,
          roomId: socket.currentRoom
        });
        console.log(`👋 User ${socket.userEmail} left room: ${socket.currentRoom}`);
      }
      
      // Remove from user socket tracking
      const userSocketIds = userSockets.get(socket.userId);
      if (userSocketIds) {
        userSocketIds.delete(socket.id);
        if (userSocketIds.size === 0) {
          userSockets.delete(socket.userId);
          console.log(`👤 User ${socket.userId} has no more sockets`);
        } else {
          console.log(`👤 User ${socket.userId} still has ${userSocketIds.size} socket(s)`);
        }
      }
    });
  });
}
