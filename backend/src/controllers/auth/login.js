// Status: Complete
// Logs in the user given their email and password
import bcrypt from 'bcrypt';
import jwt from 'jsonwebtoken';
import { UserModel } from '../../models/User.js';

const JWT_SECRET = process.env.JWT_SECRET;

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
