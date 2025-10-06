import mongoose from 'mongoose';

const roomMessageSchema = new mongoose.Schema({
  room: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'Room', 
    required: true 
  },
  sender: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'User', 
    required: true 
  },
  content: { 
    type: String, 
    required: true,
    maxlength: 2000
  },
  messageType: {
    type: String,
    enum: ['text', 'system'],
    default: 'text'
  },
  createdAt: { type: Date, default: Date.now },
  editedAt: { type: Date }
});

// Performance indexes for common queries
roomMessageSchema.index({ room: 1, createdAt: -1 }); // Room chat history
roomMessageSchema.index({ sender: 1, createdAt: -1 }); // Messages by sender
roomMessageSchema.index({ room: 1, sender: 1, createdAt: -1 }); // User messages in room

export const RoomMessageModel = mongoose.model('RoomMessage', roomMessageSchema);
