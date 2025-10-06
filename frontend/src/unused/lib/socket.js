import { io } from 'socket.io-client';

// Create socket connection with authentication
function createSocket() {
  const token = localStorage.getItem('token');
  console.log('🔌 Creating socket connection with token:', token ? 'Present' : 'Missing');
  console.log('🌐 Socket URL:', import.meta.env.VITE_API_URL);
  
  return io(import.meta.env.VITE_API_URL, {
    auth: {
      token: token
    },
    withCredentials: true
  });
}

export const socket = createSocket();

// Add connection event listeners for debugging
socket.on('connect', () => {
  console.log('✅ Socket connected:', socket.id);
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
  
  const newSocket = io(import.meta.env.VITE_API_URL, {
    auth: { token },
    withCredentials: true
  });
  
  Object.setPrototypeOf(socket, Object.getPrototypeOf(newSocket));
  Object.assign(socket, newSocket);
}

// Room management functions
export function joinRoom(roomId) {
  console.log('🚪 Attempting to join room:', roomId);
  console.log('🔌 Socket connected:', socket.connected);
  console.log('🔌 Socket ID:', socket.id);
  
  // Always try to emit
  socket.emit('join-room', roomId);
  
  // If not connected, try to connect and emit again
  if (!socket.connected) {
    console.log('🔄 Not connected, trying to connect and emit again...');
    socket.connect();
    
    setTimeout(() => {
      if (socket.connected) {
        console.log('✅ Now connected, emitting join-room again');
        socket.emit('join-room', roomId);
      }
    }, 1000);
  }
  
  return true;
}

export function leaveRoom() {
  socket.emit('leave-room');
}

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
