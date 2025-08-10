import joinRoom  from './joinRoom.js';
import leaveRoom from './leaveRoom.js';

export default function registerRoomEvents(io, socket) {
  socket.on('join-room',  roomId => joinRoom(io, socket, roomId));
  socket.on('leave-room', ()     => leaveRoom(io, socket));
}
