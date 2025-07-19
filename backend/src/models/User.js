import mongoose from 'mongoose';

const userSchema = new mongoose.Schema({
  email: { type: String, required: true, unique: true },
  username: { type: String }, // Optional, will be case-insensitive unique via explicit index
  password: { type: String, required: true }, // hashed password
  createdAt: { type: Date, default: Date.now }
});

// Create a case-insensitive unique index for username
userSchema.index({ username: 1 }, { 
  unique: true, 
  sparse: true,
  collation: { locale: 'en', strength: 2 } // Case-insensitive collation
});

export const UserModel = mongoose.model('User', userSchema);
