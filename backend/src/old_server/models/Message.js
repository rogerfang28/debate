import mongoose from 'mongoose';

const messageSchema = new mongoose.Schema({
  sender: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'User', 
    required: true 
  },
  recipient: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'User', 
    required: true 
  },
  content: { 
    type: String, 
    required: true,
    maxlength: 1000
  },
  messageType: {
    type: String,
    enum: ['text', 'room_invite'],
    default: 'text'
  },
  roomInvite: {
    roomId: { type: mongoose.Schema.Types.ObjectId, ref: 'Room' },
    roomName: String,
    inviteCode: String
  },
  isRead: { type: Boolean, default: false },
  createdAt: { type: Date, default: Date.now },
  editedAt: { type: Date }
});

// Performance indexes for common queries
messageSchema.index({ sender: 1, recipient: 1, createdAt: -1 }); // Chat history
messageSchema.index({ recipient: 1, isRead: 1, createdAt: -1 }); // Unread messages
messageSchema.index({ sender: 1, createdAt: -1 }); // Messages by sender
messageSchema.index({ recipient: 1, createdAt: -1 }); // Messages by recipient

export const MessageModel = mongoose.model('Message', messageSchema);
