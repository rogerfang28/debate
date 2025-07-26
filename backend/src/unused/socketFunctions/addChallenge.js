// // status wip
// import { ChallengeModel } from '../../models/Challenge.js';
// import { RoomModel } from '../../models/Room.js';
// import { UserModel } from '../../models/User.js';
// import { emitRoomGraph } from './utils.js';

// export async function handleChallenge(io, socket, { targetType, targetId, responder, challengeReason, room: roomId }) {
//   console.log('🎯 Challenge received:', { targetType, targetId, responder, challengeReason, roomId, challenger: socket.userId });
  
//   if (!['node', 'edge'].includes(targetType)) {
//     socket.emit('error', { message: 'Invalid target type' });
//     return;
//   }

//   try {
//     // Verify room access
//     const room = await RoomModel.findById(roomId);
//     if (!room) {
//       socket.emit('error', { message: 'Room not found' });
//       return;
//     }

//     if (!room.hasMember(socket.userId)) {
//       socket.emit('error', { message: 'Access denied - not a room member' });
//       return;
//     }

//     // Find responder user by username (case-insensitive, or email as fallback)
//     let responderUser = await UserModel.findOne({ 
//       username: { $regex: new RegExp(`^${responder}$`, 'i') }
//     });
    
//     // If not found by username, try email for backward compatibility (also case-insensitive)
//     if (!responderUser) {
//       responderUser = await UserModel.findOne({ 
//         email: { $regex: new RegExp(`^${responder}$`, 'i') }
//       });
//     }
    
//     if (!responderUser) {
//       socket.emit('error', { message: 'Responder user not found' });
//       return;
//     }

//     // Verify responder is a room member
//     if (!room.hasMember(responderUser._id)) {
//       socket.emit('error', { message: 'Responder is not a member of this room' });
//       return;
//     }

//     // Check for existing pending challenge
//     const existing = await ChallengeModel.findOne({ 
//       targetType, 
//       targetId, 
//       room: roomId,
//       status: 'pending' 
//     });
    
//     if (existing) {
//       socket.emit('error', { message: 'A pending challenge already exists for this target' });
//       return;
//     }

//     // Create challenge
//     console.log('💾 About to save challenge to database:', {
//       targetType,
//       targetId,
//       challenger: socket.userId,
//       responder: responderUser._id,
//       challengeReason: challengeReason || '',
//       room: roomId
//     });

//     const savedChallenge = await ChallengeModel.create({
//       targetType,
//       targetId,
//       challenger: socket.userId,
//       responder: responderUser._id,
//       challengeReason: challengeReason || '',
//       room: roomId
//     });

//     console.log('✅ Challenge SAVED to database with ID:', savedChallenge._id);
//     console.log('🎯 Target for deletion will be:', targetType, 'with ID:', targetId);

//     console.log(`🚩 ${targetType} challenge by ${socket.userEmail} to ${responder} in room: ${room.name}: ${challengeReason}`);
//     await emitRoomGraph(io, roomId);
    
//     socket.emit('challenge-created', { targetType, targetId });
//   } catch (err) {
//     console.error('❌ Challenge creation failed:', err);
//     socket.emit('error', { message: 'Failed to create challenge' });
//   }
// }
