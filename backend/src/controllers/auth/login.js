// * Status: Complete
// Logs in the user given their email and password

/*
*Token structure
const token = jwt.sign({
      userId,
      email,
      username,
      createdAt
}, <JWT_SECRET>, { expiresIn: '<time>' });
*/

import bcrypt from 'bcrypt';
import jwt from 'jsonwebtoken';
import { UserModel } from '../../models/User.js';

const JWT_SECRET = process.env.JWT_SECRET;

// * Login Controller
export const login = async (req, res) => {
  const { identifier, password } = req.body;

  try {
    // * Find user by email, explicitly include password field (default is select: false)
    console.log('🟡 Received identifier:', identifier);
    const user = await UserModel.findOne({
      $or: [
        { email: identifier },
        { username: new RegExp('^' + identifier + '$', 'i') }
      ]
    }).select('+password');


    // ! If no user found, return unauthorized
    if (!user) return res.status(401).json({ message: 'Invalid credentials (debug no user found)' });

    // * Compare plaintext password with hashed password from DB
    const match = await bcrypt.compare(password, user.password);

    // ! If password does not match, return unauthorized
    if (!match) return res.status(401).json({ message: 'Invalid credentials' });

    // * Generate JWT token with user info
    const token = jwt.sign(
      {
        userId: user._id,           // * Unique user ID
        email: user.email,          // * User email
        username: user.username,    // * Optional username
        createdAt: user.createdAt   // * Account creation date
      },
      JWT_SECRET,                   // * Secret key to sign the token
      { expiresIn: '7d' }           // * Token expires in 7 days
    );

    // * Send back token and user info (password excluded)
    res.status(200).json({
      token,
      userId: user._id,
      email: user.email,
      username: user.username,
      createdAt: user.createdAt
    });

  } catch (err) {
    // ! Handle unexpected server error
    console.error('🔥 Login error:', err);
    res.status(500).json({ message: 'Login failed', error: err.message });
  }
};
