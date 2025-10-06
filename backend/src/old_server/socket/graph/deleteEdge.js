import { EdgeModel } from '../../models/Edge.js';
import { RoomModel } from '../../models/Room.js';
import { emitRoomGraph } from './utils.js';

export default async function deleteEdge(io, socket, criteriaData) {
  const { room: roomId, ...criteria } = criteriaData;
  
  try {
    // Verify room access and edit permissions
    const room = await RoomModel.findById(roomId);
    if (!room) {
      socket.emit('error', { message: 'Room not found' });
      return;
    }

    if (!room.canEdit(socket.userId)) {
      socket.emit('error', { message: 'Access denied - insufficient edit permissions' });
      return;
    }

    // Find and delete the edge in this room
    const deletedEdge = await EdgeModel.findOneAndDelete({
      ...criteria,
      room: roomId
    });

    if (!deletedEdge) {
      socket.emit('error', { message: 'Edge not found in this room' });
      return;
    }

    console.log(`🗑️ Deleted edge: ${deletedEdge.source} -> ${deletedEdge.target} in room: ${room.name} by: ${socket.userEmail}`);
    await emitRoomGraph(io, roomId);
    
    socket.emit('edge-deleted', { source: deletedEdge.source, target: deletedEdge.target });
  } catch (err) {
    console.error('❌ Failed to delete edge:', err);
    socket.emit('error', { message: 'Failed to delete edge' });
  }
}
