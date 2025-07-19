import mongoose from 'mongoose';

const nodeSchema = new mongoose.Schema({
  id: { type: String, required: true },
  label: { type: String, required: true },
  metadata: { type: mongoose.Schema.Types.Mixed, default: {} },
  x: { type: Number },
  y: { type: Number },
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

// Compound index to ensure unique node IDs within each room
nodeSchema.index({ id: 1, room: 1 }, { unique: true });

// Performance indexes for common queries
nodeSchema.index({ room: 1, createdAt: -1 }); // Find nodes by room, newest first
nodeSchema.index({ createdBy: 1 }); // Find nodes by creator
nodeSchema.index({ room: 1, x: 1, y: 1 }); // Spatial queries within room

export const NodeModel = mongoose.model('Node', nodeSchema);
