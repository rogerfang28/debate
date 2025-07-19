// Status: working I guess socket is just that for now
import { io } from 'socket.io-client';
import { SOCKET_URL } from './config';

const URL = process.env.NODE_ENV === 'production'
  ? undefined
  : SOCKET_URL; // backend URL from environment

// Create socket connection with authentication
function createSocket() {
  const token = localStorage.getItem('token');
  console.log('🔌 Creating socket connection with token:', token ? 'Present' : 'Missing');
  return io(URL, {
    auth: {
      token: token
    },
    autoConnect: true,
    forceNew: true // Force new connection
  });
}

export const socket = createSocket();

// Add connection event listeners for debugging
socket.on('connect', () => {
  console.log('✅ Socket connected:', socket.id);
  console.log('🔐 Socket auth:', socket.auth);
});

socket.on('disconnect', (reason) => {
  console.log('❌ Socket disconnected:', reason);
});

socket.on('connect_error', (error) => {
  console.error('🚫 Socket connection error:', error.message);
});

// Debug all incoming events
socket.onAny((eventName, ...args) => {
  console.log('📥 Socket received event:', eventName, args);
});

// Reconnect with new token when authentication changes
export function reconnectSocket() {
  console.log('🔄 Reconnecting socket...');
  socket.disconnect();
  const token = localStorage.getItem('token');
  console.log('🔄 Reconnecting with token:', token ? 'Present' : 'Missing');
  
  // Create completely new socket instance
  const newSocket = io(URL, {
    auth: { token },
    autoConnect: true,
    forceNew: true
  });
  
  // Copy the reference to maintain the same socket object
  Object.setPrototypeOf(socket, Object.getPrototypeOf(newSocket));
  Object.assign(socket, newSocket);
  
  // Re-add debug listeners
  socket.on('connect', () => {
    console.log('✅ Socket reconnected:', socket.id);
  });
  
  socket.on('connect_error', (error) => {
    console.error('🚫 Socket reconnection error:', error.message);
  });
  
  socket.onAny((eventName, ...args) => {
    console.log('📥 Socket received event:', eventName, args);
  });
}

// Room management functions
export function joinRoom(roomId) {
  socket.emit('join-room', roomId);
}

export function leaveRoom() {
  socket.emit('leave-room');
}

// Enhanced graph operations with room context
export function addNodeToRoom(nodeData) {
  socket.emit('add-node', nodeData);
}

export function deleteNodeFromRoom(nodeData) {
  socket.emit('delete-node', nodeData);
}

export function addEdgeToRoom(edgeData) {
  socket.emit('add-edge', edgeData);
}

export function deleteEdgeFromRoom(edgeData) {
  socket.emit('delete-edge', edgeData);
}

export function updateNodePositionInRoom(positionData) {
  socket.emit('update-node-position', positionData);
}

export function challengeInRoom(challengeData) {
  socket.emit('challenge-node', challengeData);
}

export function respondToChallengeInRoom(responseData) {
  socket.emit('respond-to-challenge', responseData);
}
