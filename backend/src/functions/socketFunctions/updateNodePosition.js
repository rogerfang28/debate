import { NodeModel } from '../../models/Node.js';
import { RoomModel } from '../../models/Room.js';
import { emitRoomGraph } from './utils.js';

export async function handleUpdateNodePosition(io, socket, { id, x, y, room: roomId }) {
  console.log(`🔄 Received position update request for node ${id} at (${x}, ${y}) in room ${roomId} from user ${socket.userEmail}`);
  
  try {
    // Verify room access and edit permissions
    const room = await RoomModel.findById(roomId);
    if (!room) {
      console.error(`❌ Room not found: ${roomId}`);
      socket.emit('error', { message: 'Room not found' });
      return;
    }

    if (!room.canEdit(socket.userId)) {
      console.error(`❌ Access denied for user ${socket.userEmail} in room ${room.name}`);
      socket.emit('error', { message: 'Access denied - insufficient edit permissions' });
      return;
    }

    // Update node position in this room
    const updatedNode = await NodeModel.findOneAndUpdate(
      { id, room: roomId },
      { x, y },
      { new: true }
    );

    if (!updatedNode) {
      console.error(`❌ Node not found: ${id} in room ${roomId}`);
      socket.emit('error', { message: 'Node not found in this room' });
      return;
    }

    console.log(`🔄 Updated position for ${id}: (${x}, ${y}) in room: ${room.name} by: ${socket.userEmail}`);
    
    // Emit full graph update to all users in the room (like before)
    await emitRoomGraph(io, roomId);
  } catch (err) {
    console.error('❌ Failed to update node position:', err);
    socket.emit('error', { message: 'Failed to update node position' });
  }
}
