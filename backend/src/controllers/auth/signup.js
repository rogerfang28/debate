// Status: Complete
// Signs up a user given their email, password, and username
import bcrypt from 'bcrypt'; // Used for hashing password
import jwt from 'jsonwebtoken';
import { UserModel } from '../../models/User.js';

const JWT_SECRET = process.env.JWT_SECRET

export const signup = async (req, res) => {
  const { email, password, username } = req.body;
  try {
    const existingUser = await UserModel.findOne({ email });
    if (existingUser) return res.status(400).json({ message: 'User already exists' });

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