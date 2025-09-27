import { RoomMessageModel } from '../../models/RoomMessage.js';

export default async function sendMessage(io, socket, { message }) {
  if (!socket.currentRoom) {
    return socket.emit('error', { message: 'Join a room first' });
  }

  const msgDoc = await new RoomMessageModel({
    room: socket.currentRoom,
    sender: socket.userId,
    content: message,
    messageType: 'text'
  }).save().then(d => d.populate('sender', 'username email'));

  io.to(socket.currentRoom).emit('room-message', {
    roomId:    socket.currentRoom,
    userId:    socket.userId,
    username:  msgDoc.sender.username || msgDoc.sender.email.split('@')[0],
    message,
    timestamp: msgDoc.createdAt.toISOString(),
    messageId: msgDoc._id
  });
}
