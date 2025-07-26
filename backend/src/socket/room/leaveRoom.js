export default function leaveRoom(io, socket) {
  if (!socket.currentRoom) return;

  socket.to(socket.currentRoom).emit('user-left-room', {
    userId: socket.userId,
    userEmail: socket.userEmail,
    roomId: socket.currentRoom
  });

  socket.leave(socket.currentRoom);
  socket.currentRoom     = null;
  socket.currentRoomName = null;
}
