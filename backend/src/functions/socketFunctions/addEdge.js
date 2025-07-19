// status done
import { EdgeModel } from '../../models/Edge.js';
import { NodeModel } from '../../models/Node.js';
import { RoomModel } from '../../models/Room.js';
import { emitRoomGraph } from './utils.js';

export async function handleAddEdge(io, socket, edgeData) {
  const { room: roomId, ...edge } = edgeData;
  
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

    // Verify both nodes exist in this room
    const [sourceNode, targetNode] = await Promise.all([
      NodeModel.findOne({ id: edge.source, room: roomId }),
      NodeModel.findOne({ id: edge.target, room: roomId })
    ]);

    if (!sourceNode || !targetNode) {
      socket.emit('error', { message: 'Source or target node not found in this room' });
      return;
    }

    // Create the edge
    await EdgeModel.create({
      ...edge,
      room: roomId,
      createdBy: socket.userId
    });

    console.log(`🔗 Added edge: ${edge.source} -> ${edge.target} in room: ${room.name} by: ${socket.userEmail}`);
    await emitRoomGraph(io, roomId);
    
    socket.emit('edge-added', { source: edge.source, target: edge.target });
  } catch (err) {
    console.error('❌ Failed to add edge:', err);
    socket.emit('error', { message: 'Failed to add edge' });
  }
}
