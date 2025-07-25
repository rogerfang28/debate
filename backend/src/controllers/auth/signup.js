// * Status: Complete
// Signs up a user given their email, password, and username
import bcrypt from 'bcrypt'; // Used for hashing password
import jwt from 'jsonwebtoken';
import { UserModel } from '../../models/User.js';

const JWT_SECRET = process.env.JWT_SECRET

// * Signup Controller
export const signup = async (req, res) => {
  const { email, password, username } = req.body;

  try {
    // * Check if a user with the same email already exists
    const existingUser = await UserModel.findOne({ email });

    // ! If email already in use, return a 400 Bad Request
    if (existingUser) return res.status(400).json({ message: 'User already exists' });

    // * If a username is provided, check if it's already taken (case-insensitive)
    if (username && username.trim()) {
      const trimmedUsername = username.trim();

      // * Regex with 'i' flag for case-insensitive match (e.g., "John" == "john")
      const existingUsername = await UserModel.findOne({
        username: { $regex: new RegExp(`^${trimmedUsername}$`, 'i') }
      });

      // ! If username is already taken, return error
      if (existingUsername) {
        return res.status(400).json({ message: 'Username is already taken' });
      }
    }

    // * Hash the password with bcrypt (salt rounds = 10)
    const hashed = await bcrypt.hash(password, 10);

    // * Prepare user data object for saving
    const userData = { email, password: hashed };
    if (username && username.trim()) {
      userData.username = username.trim();
    }

    // * Create new user in the database
    const user = await UserModel.create(userData);

    // * Generate JWT token with user info
    const token = jwt.sign(
      {
        userId: user._id,
        email: user.email,
        username: user.username,
        createdAt: user.createdAt
      },
      JWT_SECRET,            // * Secret key for signing
      { expiresIn: '7d' }     // * Token expires in 7 days
    );

    // * Send response with created user info and token
    res.status(201).json({
      message: 'User created',     // * Success message
      token,                       // * Auth token for frontend to store
      userId: user._id,
      email: user.email,
      username: user.username,
      createdAt: user.createdAt
    });

  } catch (err) {
    // ! If any error occurs during signup, return 500 Internal Server Error
    res.status(500).json({ message: 'Signup failed', error: err.message });
  }
};
