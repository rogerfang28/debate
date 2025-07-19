import mongoose from 'mongoose';

const friendSchema = new mongoose.Schema({
  requester: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'User', 
    required: true 
  },
  recipient: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'User', 
    required: true 
  },
  status: {
    type: String,
    enum: ['pending', 'accepted', 'declined', 'blocked'],
    default: 'pending'
  },
  createdAt: { type: Date, default: Date.now },
  updatedAt: { type: Date, default: Date.now }
});

// Ensure no duplicate friend requests between same users
friendSchema.index({ requester: 1, recipient: 1 }, { unique: true });

// Performance indexes for common queries
friendSchema.index({ requester: 1, status: 1 }); // Find outgoing requests by status
friendSchema.index({ recipient: 1, status: 1 }); // Find incoming requests by status
friendSchema.index({ status: 1, createdAt: -1 }); // Find requests by status and date

// Update timestamp on save
friendSchema.pre('save', function(next) {
  this.updatedAt = new Date();
  next();
});

export const FriendModel = mongoose.model('Friend', friendSchema);
