// import { NodeModel } from '../../models/Node.js';
// import { EdgeModel } from '../../models/Edge.js';
// import { RoomModel } from '../../models/Room.js';
// import { emitRoomGraph } from './utils.js';

// export async function handleDeleteNode(io, socket, { id, room: roomId }) {
//   try {
//     // Verify room access and edit permissions
//     const room = await RoomModel.findById(roomId);
//     if (!room) {
//       socket.emit('error', { message: 'Room not found' });
//       return;
//     }

//     if (!room.canEdit(socket.userId)) {
//       socket.emit('error', { message: 'Access denied - insufficient edit permissions' });
//       return;
//     }

//     // Find the node to verify it exists in this room
//     const node = await NodeModel.findOne({ id, room: roomId });
//     if (!node) {
//       socket.emit('error', { message: 'Node not found in this room' });
//       return;
//     }

//     // Delete node and related edges
//     await Promise.all([
//       NodeModel.deleteOne({ id, room: roomId }),
//       EdgeModel.deleteMany({ 
//         room: roomId,
//         $or: [{ source: id }, { target: id }] 
//       })
//     ]);

//     console.log(`🗑️ Deleted node and related edges: ${id} in room: ${room.name} by: ${socket.userEmail}`);
//     await emitRoomGraph(io, roomId);
    
//     socket.emit('node-deleted', { nodeId: id });
//   } catch (err) {
//     console.error('❌ Failed to delete node:', err);
//     socket.emit('error', { message: 'Failed to delete node' });
//   }
// }
