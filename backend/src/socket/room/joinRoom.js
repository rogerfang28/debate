import { RoomModel } from '../../models/Room.js';
import { RoomMessageModel } from '../../models/RoomMessage.js';

/** join‑room event */
export default async function joinRoom(io, socket, roomId) {
  /* 1. Verify membership */
  const room = await RoomModel.findById(roomId);
  if (!room || !room.hasMember(socket.userId)) {
    return socket.emit('error', { message: 'Access denied' });
  }

  /* 2. Join socket.io room */
  socket.join(roomId);
  socket.currentRoom      = roomId;
  socket.currentRoomName  = room.name;

  /* 3. Notify others */
  socket.to(roomId).emit('user-joined-room', {
    userId: socket.userId,
    userEmail: socket.userEmail,
    roomId
  });

  /* 4. Send chat history + online users back to this client */
  const roomSockets = await io.in(roomId).fetchSockets();
  const onlineUsers = roomSockets.map(s => ({
    userId: s.userId, userEmail: s.userEmail, socketId: s.id
  }));

  const history = await RoomMessageModel.find({ room: roomId })
    .populate('sender', 'username email')
    .sort({ createdAt: -1 })
    .limit(50).lean();

  socket.emit('joined-room', {
    roomId,
    roomName: room.name,
    onlineUsers,
    messages: history.reverse().map(m => ({
      roomId,
      userId:   m.sender._id,
      username: m.sender.username || m.sender.email.split('@')[0],
      message:  m.content,
      timestamp:m.createdAt.toISOString(),
      messageId:m._id
    }))
  });
}
