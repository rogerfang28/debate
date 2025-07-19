import mongoose from 'mongoose';

const roomSchema = new mongoose.Schema({
  name: { 
    type: String, 
    required: true,
    trim: true,
    maxlength: 100
  },
  description: { 
    type: String, 
    trim: true,
    maxlength: 500,
    default: ''
  },
  owner: { 
    type: mongoose.Schema.Types.ObjectId, 
    ref: 'User', 
    required: true 
  },
  members: [{
    user: { 
      type: mongoose.Schema.Types.ObjectId, 
      ref: 'User', 
      required: true 
    },
    role: { 
      type: String, 
      enum: ['owner', 'admin', 'member'], 
      default: 'member' 
    },
    joinedAt: { 
      type: Date, 
      default: Date.now 
    }
  }],
  inviteCode: { 
    type: String, 
    unique: true,
    sparse: true // allows null values while maintaining uniqueness for non-null values
  },
  isPublic: { 
    type: Boolean, 
    default: false 
  },
  maxMembers: { 
    type: Number, 
    default: 50,
    min: 1,
    max: 200
  },
  settings: {
    allowMemberInvites: { type: Boolean, default: true },
    allowMemberEdit: { type: Boolean, default: true },
    requireApproval: { type: Boolean, default: false }
  },
  createdAt: { 
    type: Date, 
    default: Date.now 
  },
  updatedAt: { 
    type: Date, 
    default: Date.now 
  }
});

// Performance Indexes - Critical for scaling
roomSchema.index({ owner: 1 }); // Find rooms by owner
roomSchema.index({ isPublic: 1, createdAt: -1 }); // Find public rooms, newest first
roomSchema.index({ 'members.user': 1 }); // Find rooms by member
roomSchema.index({ createdAt: -1 }); // Sort by creation date
roomSchema.index({ inviteCode: 1 }, { sparse: true }); // Invite code lookup

// Update the updatedAt field before saving
roomSchema.pre('save', function(next) {
  this.updatedAt = Date.now();
  next();
});

// Generate a unique invite code when needed
roomSchema.methods.generateInviteCode = function() {
  const characters = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789';
  let result = '';
  for (let i = 0; i < 8; i++) {
    result += characters.charAt(Math.floor(Math.random() * characters.length));
  }
  this.inviteCode = result;
  return result;
};

// Check if user is member of the room
roomSchema.methods.hasMember = function(userId) {
  return this.members.some(member => {
    // Handle both populated and non-populated user fields
    const memberUserId = member.user._id || member.user;
    return memberUserId.toString() === userId.toString();
  });
};

// Get user's role in the room
roomSchema.methods.getMemberRole = function(userId) {
  const member = this.members.find(member => {
    // Handle both populated and non-populated user fields
    const memberUserId = member.user._id || member.user;
    return memberUserId.toString() === userId.toString();
  });
  return member ? member.role : null;
};

// Check if user can edit the room
roomSchema.methods.canEdit = function(userId) {
  const role = this.getMemberRole(userId);
  return role === 'owner' || role === 'admin' || (role === 'member' && this.settings.allowMemberEdit);
};

export const RoomModel = mongoose.model('Room', roomSchema);
