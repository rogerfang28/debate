import sendMessage from './sendMessage.js';

export default function registerChatEvents(io, socket) {
  socket.on('room-message', p => sendMessage(io, socket, p));
}
