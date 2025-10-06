// * Status: Complete
// * Updates the user's profile with a new username

import jwt from 'jsonwebtoken';
import { UserModel } from '../../models/User.js';

const JWT_SECRET = process.env.JWT_SECRET || 'secret123';

// * Controller to handle profile updates
export const updateProfile = async (req, res) => {
  try {
    const { username } = req.body; // * Get the new username from the request body
    const userId = req.user.userId; // * Extract user ID from the JWT token (middleware must populate req.user)

    // ! Username is required and cannot be just whitespace
    if (!username || !username.trim()) {
      return res.status(400).json({ message: 'Username is required' });
    }

    const trimmedUsername = username.trim();

    // ! Validate username length
    if (trimmedUsername.length < 3 || trimmedUsername.length > 30) {
      return res.status(400).json({ message: 'Username must be 3–30 characters long' });
    }

    // ! Validate allowed characters (letters, numbers, underscores, hyphens)
    if (!/^[a-zA-Z0-9_-]+$/.test(trimmedUsername)) {
      return res.status(400).json({ message: 'Invalid username format' });
    }

    // * Check if username is taken by another user (case-insensitive)
    const existingUser = await UserModel.findOne({
      username: { $regex: new RegExp(`^${trimmedUsername}$`, 'i') },
      _id: { $ne: userId } // * Exclude the current user from the check
    });

    // ! Reject if the username is already in use
    if (existingUser) {
      return res.status(400).json({ message: 'Username is already taken' });
    }

    // * Update the user's username and return the new user object (excluding password)
    const updatedUser = await UserModel.findByIdAndUpdate(
      userId,
      { username: trimmedUsername },
      { new: true, select: '-password' } // * Return updated document, exclude password field
    );

    // ! Handle case where user no longer exists
    if (!updatedUser) {
      return res.status(404).json({ message: 'User not found' });
    }

    // * Create a new JWT with updated user info
    const token = jwt.sign(
      {
        userId: updatedUser._id,
        email: updatedUser.email,
        username: updatedUser.username,
        createdAt: updatedUser.createdAt
      },
      JWT_SECRET,
      { expiresIn: '1h' } // * Token expires in 1 hour
    );

    // * Send back updated user info and new token
    res.status(200).json({
      message: 'Profile updated successfully',
      token,
      userId: updatedUser._id,
      email: updatedUser.email,
      username: updatedUser.username,
      createdAt: updatedUser.createdAt
    });

  } catch (err) {
    // ! Catch and report unexpected errors
    res.status(500).json({ message: 'Profile update failed', error: err.message });
  }
};
