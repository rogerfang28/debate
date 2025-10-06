import mongoose from 'mongoose';

const edgeSchema = new mongoose.Schema({
  source: { type: String, required: true },
  target: { type: String, required: true },
  relation: { type: String, default: '' },
  metadata: { type: mongoose.Schema.Types.Mixed, default: {} },
  room: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'Room', 
    required: true 
  },
  createdBy: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'User', 
    required: true 
  },
  createdAt: { 
    type: Date, 
    default: Date.now 
  }
});

// Performance indexes for common queries
edgeSchema.index({ room: 1, createdAt: -1 }); // Find edges by room, newest first
edgeSchema.index({ source: 1, target: 1, room: 1 }, { unique: true }); // Prevent duplicate edges
edgeSchema.index({ createdBy: 1 }); // Find edges by creator
edgeSchema.index({ source: 1, room: 1 }); // Find edges from a node
edgeSchema.index({ target: 1, room: 1 }); // Find edges to a node

export const EdgeModel = mongoose.model('Edge', edgeSchema);
