// import { NodeModel } from '../../models/Node.js';
// import { RoomModel } from '../../models/Room.js';
// import { emitRoomGraph } from './utils.js';

// export async function handleAddNode(io, socket, nodeData) {
//   const { room: roomId, ...node } = nodeData;
  
//   if (typeof node.label !== 'string') {
//     console.warn('❌ Rejected node: label must be a string', node);
//     socket.emit('error', { message: 'Node label must be a string' });
//     return;
//   }

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

//     // Create the node
//     await NodeModel.create({
//       id: node.id,
//       label: node.label,
//       metadata: node.metadata || {},
//       x: typeof node.x === 'number' ? node.x : 0,
//       y: typeof node.y === 'number' ? node.y : 0,
//       room: roomId,
//       createdBy: socket.userId
//     });

//     console.log(`✅ Added node: ${node.id} in room: ${room.name} by: ${socket.userEmail}`);
    
//     // Emit to all users in the room
//     await emitRoomGraph(io, roomId);
    
//     // Send success confirmation to the creator
//     socket.emit('node-added', { nodeId: node.id });
//   } catch (err) {
//     console.error('❌ Failed to add node:', err);
//     socket.emit('error', { message: 'Failed to add node' });
//   }
// }
