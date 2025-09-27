// models/Challenge.js
import mongoose from 'mongoose';

const challengeSchema = new mongoose.Schema({
  targetType: { type: String, enum: ['node', 'edge'], required: true },
  targetId: { type: String, required: true },
  challenger: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'User', 
    required: true 
  },
  responder: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'User', 
    required: true 
  },
  challengeReason: { type: String, default: '' }, 
  status: { type: String, enum: ['pending', 'accepted', 'denied'], default: 'pending' },
  resolutionReason: { type: String, default: '' },
  room: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'Room', 
    required: true 
  },
  createdAt: { type: Date, default: Date.now }
});

// Index for efficient room-based queries
challengeSchema.index({ room: 1 });
challengeSchema.index({ responder: 1, status: 1 });

export const ChallengeModel = mongoose.model('Challenge', challengeSchema);

/*
Examples:
{
  targetType: 'node',
  targetId: 'node-123',
  challenger: ObjectId('...'), // User ObjectId
  responder: ObjectId('...'),  // User ObjectId
  room: ObjectId('...'),       // Room ObjectId
  status: 'pending'
}

{
  targetType: 'edge',
  targetId: '660f13d2d1cf6aee09345c21', // _id of the edge document
  challenger: ObjectId('...'),
  responder: ObjectId('...'),
  room: ObjectId('...'),
  status: 'pending'
}

*/