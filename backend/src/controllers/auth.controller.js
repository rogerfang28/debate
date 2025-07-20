import bcrypt from 'bcrypt';
import jwt from 'jsonwebtoken';
import { UserModel } from '../models/User.js';

const JWT_SECRET = process.env.JWT_SECRET || 'secret123';

export const signup = async (req, res) => {
  const { email, password, username } = req.body;
  try {
    const existingUser = await UserModel.findOne({ email });
    if (existingUser) return res.status(400).json({ message: 'User already exists' });

    // Check if username is provided and if it's already taken (case-insensitive)
    if (username && username.trim()) {
      const trimmedUsername = username.trim();
      const existingUsername = await UserModel.findOne({ 
        username: { $regex: new RegExp(`^${trimmedUsername}$`, 'i') }
      });
      if (existingUsername) {
        return res.status(400).json({ message: 'Username is already taken' });
      }
    }

    const hashed = await bcrypt.hash(password, 10);
    const userData = { email, password: hashed };
    if (username && username.trim()) {
      userData.username = username.trim();
    }
    const user = await UserModel.create(userData);

    // Create token with user info for immediate login
    const token = jwt.sign({ 
      userId: user._id, 
      email: user.email,
      username: user.username,
      createdAt: user.createdAt
    }, JWT_SECRET, { expiresIn: '7d' });

    res.status(201).json({ 
      message: 'User created', 
      token,
      userId: user._id,
      email: user.email,
      username: user.username,
      createdAt: user.createdAt
    });
  } catch (err) {
    res.status(500).json({ message: 'Signup failed', error: err.message });
  }
};

export const login = async (req, res) => {
  const { email, password } = req.body;
  try {
    const user = await UserModel.findOne({ email }).select('+password');
    if (!user) return res.status(401).json({ message: 'Invalid credentials' });

    const match = await bcrypt.compare(password, user.password);
    if (!match) return res.status(401).json({ message: 'Invalid credentials' });

    const token = jwt.sign({ 
      userId: user._id, 
      email: user.email,
      username: user.username,
      createdAt: user.createdAt
    }, JWT_SECRET, { expiresIn: '7d' });

    res.status(200).json({ 
      token, 
      userId: user._id,
      email: user.email,
      username: user.username,
      createdAt: user.createdAt
    });
  } catch (err) {
    res.status(500).json({ message: 'Login failed', error: err.message });
  }
};

export const logout = (req, res) => {
  // With JWT, logout is handled client-side by deleting the token
  res.status(200).json({ message: 'Logged out' });
};

export const updateProfile = async (req, res) => {
  try {
    const { username } = req.body;
    const userId = req.user.userId; // Get from JWT token via auth middleware

    console.log('🔍 Update profile request:', { username, userId, userFromToken: req.user });

    if (!username || !username.trim()) {
      return res.status(400).json({ message: 'Username is required' });
    }

    // Validate username format
    const trimmedUsername = username.trim();
    if (trimmedUsername.length < 3) {
      return res.status(400).json({ message: 'Username must be at least 3 characters long' });
    }
    if (trimmedUsername.length > 30) {
      return res.status(400).json({ message: 'Username must be less than 30 characters long' });
    }
    if (!/^[a-zA-Z0-9_-]+$/.test(trimmedUsername)) {
      return res.status(400).json({ message: 'Username can only contain letters, numbers, underscores, and hyphens' });
    }

    // Check if username is already taken by another user (case-insensitive)
    const existingUser = await UserModel.findOne({ 
      username: { $regex: new RegExp(`^${trimmedUsername}$`, 'i') }, 
      _id: { $ne: userId } 
    });
    
    if (existingUser) {
      return res.status(400).json({ message: 'Username is already taken' });
    }

    // Update the user
    const updatedUser = await UserModel.findByIdAndUpdate(
      userId,
      { username: trimmedUsername },
      { new: true, select: '-password' }
    );

    console.log('📝 User updated:', updatedUser);

    if (!updatedUser) {
      return res.status(404).json({ message: 'User not found' });
    }

    // Create a new token with updated user info
    const token = jwt.sign(
      { 
        userId: updatedUser._id, 
        email: updatedUser.email,
        username: updatedUser.username,
        createdAt: updatedUser.createdAt
      }, 
      JWT_SECRET, 
      { expiresIn: '1h' }
    );

    res.status(200).json({
      message: 'Profile updated successfully',
      token,
      userId: updatedUser._id,
      email: updatedUser.email,
      username: updatedUser.username,
      createdAt: updatedUser.createdAt
    });
  } catch (err) {
    console.error('Profile update error:', err);
    res.status(500).json({ message: 'Profile update failed', error: err.message });
  }
};
