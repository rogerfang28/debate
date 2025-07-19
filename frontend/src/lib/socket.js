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
