// Status: Complete
// Updates the user's profile given their new username
import jwt from 'jsonwebtoken';
import { UserModel } from '../../models/User.js';

const JWT_SECRET = process.env.JWT_SECRET || 'secret123';

export const updateProfile = async (req, res) => {
  try {
    const { username } = req.body; // Input is the username
    const userId = req.user.userId;

    if (!username || !username.trim()) {
      return res.status(400).json({ message: 'Username is required' });
    }

    const trimmedUsername = username.trim();
    if (trimmedUsername.length < 3 || trimmedUsername.length > 30) {
      return res.status(400).json({ message: 'Username must be 3–30 characters long' });
    }

    if (!/^[a-zA-Z0-9_-]+$/.test(trimmedUsername)) {
      return res.status(400).json({ message: 'Invalid username format' });
    }

    const existingUser = await UserModel.findOne({
      username: { $regex: new RegExp(`^${trimmedUsername}$`, 'i') },
      _id: { $ne: userId }
    });

    if (existingUser) {
      return res.status(400).json({ message: 'Username is already taken' });
    }

    const updatedUser = await UserModel.findByIdAndUpdate(
      userId,
      { username: trimmedUsername },
      { new: true, select: '-password' }
    );

    if (!updatedUser) {
      return res.status(404).json({ message: 'User not found' });
    }

    const token = jwt.sign({
      userId: updatedUser._id,
      email: updatedUser.email,
      username: updatedUser.username,
      createdAt: updatedUser.createdAt
    }, JWT_SECRET, { expiresIn: '1h' });

    res.status(200).json({
      message: 'Profile updated successfully',
      token,
      userId: updatedUser._id,
      email: updatedUser.email,
      username: updatedUser.username,
      createdAt: updatedUser.createdAt
    });
  } catch (err) {
    res.status(500).json({ message: 'Profile update failed', error: err.message });
  }
};
