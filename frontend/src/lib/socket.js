// Status: working I guess socket is just that for now
import { io } from 'socket.io-client';

const URL = process.env.NODE_ENV === 'production'
  ? undefined
  : 'http://localhost:3000'; // backend URL during dev

// Create socket connection with authentication
function createSocket() {
  const token = localStorage.getItem('token');
  return io(URL, {
    auth: {
      token: token
    }
  });
}

export const socket = createSocket();

// Reconnect with new token when authentication changes
export function reconnectSocket() {
  socket.disconnect();
  const newSocket = createSocket();
  // Copy the reference to maintain the same socket object
  Object.setPrototypeOf(socket, Object.getPrototypeOf(newSocket));
  Object.assign(socket, newSocket);
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
